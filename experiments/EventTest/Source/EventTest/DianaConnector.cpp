// Fill out your copyright notice in the Description page of Project Settings.

#include "EventTest.h"
#include "DianaConnector.h"

#include "Array.h"

#include "Networking.h"
#include "Sockets.h"
#include "SocketTypes.h"
#include "SocketSubsystem.h"

#include "messaging.hpp"

#include <string.h>
#include <thread>
#include <list>

// See: https://wiki.unrealengine.com/Multi-Threading:_How_to_Create_Threads_in_UE4
class FVisDataReceiver : public FRunnable
{
public:
    FVisDataReceiver(FSocket* sock, ADianaConnector* parent, std::map<int32, struct ADianaConnector::DianaActor*>* oa_map);
    ~FVisDataReceiver();

    virtual bool Init();
    virtual uint32 Run();
    virtual void Stop();

private:
    FRunnableThread* rt = NULL;
    volatile bool running;
    FSocket* sock = NULL;
    ADianaConnector* parent;
    std::map<int32, struct ADianaConnector::DianaActor*>* oa_map;
};

FVisDataReceiver::FVisDataReceiver(FSocket* sock, ADianaConnector* parent, std::map<int32, struct ADianaConnector::DianaActor*>* oa_map)
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Constructor"));
    this->sock = sock;
    this->parent = parent;
    this->oa_map = oa_map;
    rt = FRunnableThread::Create(this, TEXT("VisDataReceiver"), 0, EThreadPriority::TPri_Normal); // Don't set an affinity mask
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Constructor::PostThreadCreate"));
}

FVisDataReceiver::~FVisDataReceiver()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Destructor"));
    Stop();
    delete rt;
    rt = NULL;
}

bool FVisDataReceiver::Init()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Init"));
    running = true;
    return true;
}

void motion_interpolation(double* t, FVector* p, FVector& v, FVector& a)
{
    FVector dp[] = { p[1] - p[0], p[2] - p[0] };
    double dt[] = { t[1] - t[0], t[2] - t[0] };

    a = dp[0] * (2.0 / (dt[0] * (dt[0] - dt[1]))) -
        dp[1] * (2.0 / (dt[1] * (dt[0] - dt[1])));

    //v = 0.5 * (dp[1] * (dt[0] / (dt[1] * (dt[0] - dt[1]))) -
    //    dp[0] * (dt[1] / (dt[0] * (dt[0] - dt[1])))) + (p[2] - p[1]) / (t[2] - t[1]);

    v = dp[1] * (dt[0] / (dt[1] * (dt[0] - dt[1]))) -
        dp[0] * (dt[1] / (dt[0] * (dt[0] - dt[1])));

    //v = (p[2] - p[1]) / (t[2] - t[1]);
    //a.X = 0.0;
    //a.Y = 0.0;
    //a.Z = 0.0;
}

uint32 FVisDataReceiver::Run()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Run::Begin"));

    std::chrono::milliseconds dura(10);
    // Timespan representing 1 second as 10-million 0.1us (100ns) ticks.
    FTimespan sock_wait_time(10000000);
    bool read_available = false;
    Diana::BSONMessage* m = NULL;
    Diana::VisualDataMsg* vdm = NULL;
    struct ADianaConnector::DianaVDM dm;
    struct ADianaConnector::DianaActor* da;
    std::map<int32, struct ADianaConnector::DianaActor*>::iterator mit;
    FVector velocity, acceleration;
    uint32 nmessages = 0;
    UWorld* world = parent->GetWorld();
    float last_stat_time = world->RealTimeSeconds;
    int last_stat_frames = 0;

    // Hold a list of all DianaActor struct pointers, we're going to use it like a stack
    // to make it easier to expire objects at the end of a vis frame.
    std::list<struct ADianaConnector::DianaActor*> last_seen;
    std::list<struct ADianaConnector::DianaActor*>::iterator lit;
    int64 vis_iterations = 1;

    // The number of UU per metre in the Unreal map that the parent actor lives in.
    // We blindly assume that all EPC (or at least their associated static meshes/actors
    // exist in the same map, or at least have the same scale, as the DianaConnector.
    //
    // Divide by 2.... because. Not sure why, but we need to.
    float world_to_metres = world->GetWorldSettings()->WorldToMeters / 2.0;

    while (running)
    {
        read_available = sock->Wait(ESocketWaitConditions::Type::WaitForRead, sock_wait_time);
        if (read_available)
        {
            m = Diana::BSONMessage::BSONMessage::ReadMessage(sock);

            // Only consider messages that are VisualData, and have our client_id, and have the phys_id specced[2]
            if ((m != NULL) &&
                (m->msg_type == Diana::BSONMessage::VisualData) &&
                m->specced[0] && m->specced[1] && (m->client_id == parent->client_id))
            {
                // If the phys_id is specced, then use it.
                if (m->specced[2])
                {
                    vdm = (Diana::VisualDataMsg*)m;
                    dm.server_id = (int32)vdm->phys_id & 0x7FFFFFFF; // Unsign the ID, because that's natural.
                    dm.world_time = world->RealTimeSeconds;
                    dm.radius = vdm->radius;
                    dm.pos = world_to_metres * FVector(vdm->position.x, vdm->position.y, vdm->position.z);

                    {
                        FScopeLock(&parent->map_cs);
                        mit = oa_map->find(dm.server_id);
                    }

                    if (mit == oa_map->end())
                    {
                        da = new struct ADianaConnector::DianaActor;
                        // The object is new
                        da->time[0] = dm.world_time - 2.0;
                        da->time[1] = dm.world_time - 1.0;
                        da->time[2] = dm.world_time;
                        da->pos[0] = dm.pos;
                        da->pos[1] = dm.pos;
                        da->pos[2] = dm.pos;
                        da->radius = dm.radius;
                        da->server_id = dm.server_id;
                        da->last_iteration = vis_iterations;
                        da->a = NULL;
                        da->epc = NULL;

                        // Add this actor to the last_seen list, at the end, because it was the last seen.
                        last_seen.push_back(da);
                        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::ListLength %u"), last_seen.size());

                        dm.da = NULL;
                        parent->messages.Enqueue(dm);

                        FScopeLock Lock(&parent->map_cs);
                        oa_map->insert(std::pair<int32, struct ADianaConnector::DianaActor*>(dm.server_id, da));
                    }
                    else
                    {
                        da = mit->second;
                        da->time[0] = da->time[1];
                        da->time[1] = da->time[2];
                        da->time[2] = dm.world_time;
                        da->pos[0] = da->pos[1];
                        da->pos[1] = da->pos[2];
                        da->pos[2] = dm.pos;
                        da->last_iteration = vis_iterations;

                        // Find this object in the list, and move it to the end.
                        // Because objects are, generally, being sent in the same order each frame
                        // (or, subsequent frames are very clode in order), this should be a
                        // pretty short linear search each time. Using the linked list will
                        // make unlinking and pushing to the back easier on large lists.
                        for (lit = last_seen.begin(); lit != last_seen.end(); lit++)
                        {
                            if ((*lit)->server_id == da->server_id)
                            {
                                last_seen.erase(lit);
                                break;
                            }
                        }

                        last_seen.push_back(mit->second);

                        if (da->epc != NULL)
                        {
                            motion_interpolation(da->time, da->pos, velocity, acceleration);
                            da->epc->SetPVA(da->pos[2], velocity, acceleration);
                        }
                    }
                }
                // If the phys_id is unspecced, then we need to check up on our list, and see
                // which objects missed their update, indicating they should be removed from
                // the scene.
                else
                {
                    for (lit = last_seen.begin(); ((lit != last_seen.end()) && ((*lit)->last_iteration < vis_iterations)); )
                    {
                        // In this case, just set the da pointer to be to the object that
                        // is expired, the non-NULL-ness will trigger the removal, and the
                        // rest of the dm struct will be ignored.
                        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Erasing %d"), (*lit)->server_id);
                        dm.da = *lit;
                        dm.server_id = (*lit)->server_id;
                        parent->messages.Enqueue(dm);
                        lit = last_seen.erase(lit);
                    }

                    vis_iterations++;
                    last_stat_frames++;
                }

                if ((world->RealTimeSeconds - last_stat_time) >= 1.0f)
                {
                    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Stats %d"), last_stat_frames);
                    last_stat_frames = 0;
                    last_stat_time = world->RealTimeSeconds;
                }

                // We handled a message, so immediately repeat, skip the Sleep()
                delete vdm;
                continue;
            }

            if (m != NULL)
            {
                delete m;
            }

            std::this_thread::sleep_for(dura);
        }
    }
    return nmessages;
}

void FVisDataReceiver::Stop()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Stop"));
    running = false;
    rt->WaitForCompletion();
}

bool ADianaConnector::ConnectToServer(FString _host, int32 _port)
{
    this->host = _host;
    this->port = _port;

    if (sock != NULL)
    {
        DisconnectSocket();
    }

    return ConnectSocket();
}

bool ADianaConnector::ConnectSocket()
{
    if ((sock == NULL) && (port > 1) && (port < 65535) && (host.Len() >= 8))
    {
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::ConnectSocket"));
        sock = FTcpSocketBuilder(TEXT("xSIMConn"));
        FIPv4Address ip;
        FIPv4Address::Parse(host, ip);
        FIPv4Endpoint endpoint(ip, (uint16)port);
        bool connected = sock->Connect(*endpoint.ToInternetAddr());
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::ConnectSocket::%d"), connected);
        return connected;
    }
    else
    {
        if ((sock != NULL) &&
            (sock->GetConnectionState() == ESocketConnectionState::SCS_Connected))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

void ADianaConnector::DisconnectSocket()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::DisconnectSocket"));
    if (sock != NULL)
    {
        sock->Close();
        delete sock;
        sock = NULL;
    }
}

// Sets default values
ADianaConnector::ADianaConnector()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
    this->proxy = this;
    this->sock = NULL;
}

ADianaConnector::~ADianaConnector()
{
    RegisterForVisData(false);
    DisconnectSocket();
}

// Called when the game starts or when spawned
void ADianaConnector::BeginPlay()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging:BeginPlay"));
    Super::BeginPlay();

}

// Called every frame
void ADianaConnector::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    struct DianaVDM dm;
    struct DianaActor* da;

    while (!messages.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::Tick::HandlingQueueMessage"));
        messages.Dequeue(dm);

        // If the object disappeared, call out to Blueprints to handle that event
        if (dm.da != NULL)
        {
            da = dm.da;
            RemovedVisDataObject(da->server_id, da->a, da->epc);
            FScopeLock(&this->map_cs);
            oa_map.erase(dm.server_id);
            delete dm.da;
        }
        // Otherweise call out to Blueprints to handle a new object.
        else
        {
            // The object is new
            NewVisDataObject(dm.server_id, dm.radius, dm.pos);
        }
    }
}

bool ADianaConnector::RegisterForVisData(bool enable)
{
    return proxy->RegisterForVisData(enable, client_id, server_id);
}

bool ADianaConnector::RegisterForVisData(bool enable, int32 client_id, int32 server_id)
{
    // If we're disconnected (NULL worker thread), and trying to disconnect again, just do nothing.
    if ((vdr_thread == NULL) && !enable)
    {
        return true;
    }

    ConnectSocket();

    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataToggle::Begin (%d) %d %d"), enable, client_id, server_id);
    if (sock == NULL)
    {
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataToggle::NULLSocket"));
        return false;
    }

    Diana::VisualDataEnableMsg vdm;
    vdm.enabled = enable;
    vdm.client_id = client_id;
    vdm.server_id = server_id;
    vdm.spec_all();

    if (enable)
    {
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataToggle::Enable"));
        vdr_thread = new FVisDataReceiver(sock, this, &oa_map);
        vdm.send(sock);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataToggle::Disable"));
        vdm.send(sock);

        if (vdr_thread != NULL)
        {
            UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataToggle:Disable::ThreadStop %p"), (void*)vdr_thread);
            vdr_thread->Stop();
            DisconnectSocket();
            delete vdr_thread;
            vdr_thread = NULL;
        }
    }

    return true;
}

void ADianaConnector::UseProxyConnection(ADianaConnector* _proxy)
{
    this->proxy = _proxy;
    this->client_id = _proxy->client_id + 1;
}

void ADianaConnector::UpdateExistingVisDataObject(int32 PhysID, AActor* ActorRef, UExtendedPhysicsComponent* EPCRef)
{
    FScopeLock Lock(&map_cs);
    oa_map[PhysID]->a = ActorRef;
    oa_map[PhysID]->epc = EPCRef;
}

TArray<struct FDirectoryItem> ADianaConnector::DirectoryListing(FString type, TArray<struct FDirectoryItem> items)
{
    return proxy->DirectoryListing(client_id, server_id, type, items);
}

void ADianaConnector::CreateShip(int32 class_id)
{
    proxy->CreateShip(client_id, server_id, class_id);
}

int32 ADianaConnector::JoinShip(int32 ship_id)
{
    int32 ret = proxy->JoinShip(client_id, server_id, ship_id);
    server_id = ret;
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::JoinedShip %u"), server_id);
    return ret;
}

void ADianaConnector::RenameShip(FString new_name)
{
    proxy->RenameShip(client_id, server_id, new_name);
}

void ADianaConnector::Ready()
{
    proxy->Ready(client_id, server_id);
}

void ADianaConnector::Goodbye()
{
    proxy->Goodbye(client_id, server_id);
}

char* fstring_to_char(FString s)
{
    char* src = TCHAR_TO_ANSI(*s);
    size_t src_len = strnlen(src, s.Len());
    char* dst = (char*)malloc(src_len);
    if (dst == NULL)
    {
        throw std::runtime_error("UnableToAllocateDirItemType");
    }
    memcpy(dst, src, src_len + 1);
    return dst;
}

TArray<struct FDirectoryItem> ADianaConnector::DirectoryListing(int32 client_id, int32 server_id, FString type, TArray<struct FDirectoryItem> items)
{
    ConnectSocket();

    Diana::DirectoryMsg dm;
    dm.specced[0] = true;
    dm.specced[1] = true;
    dm.specced[2] = true;
    dm.specced[3] = true;

    dm.server_id = server_id;
    dm.client_id = client_id;
    dm.item_count = items.Num();
    dm.item_type = fstring_to_char(type);

    if (dm.item_count > 0)
    {
        dm.specced[4] = true;
        dm.specced[5] = true;

        dm.items = new Diana::DirectoryMsg::DirectoryItem[dm.item_count];
        for (int i = 0; i < dm.item_count; i++)
        {
            dm.items[i].id = items[i].id;
            dm.items[i].name = fstring_to_char(items[i].name);
        }
    }

    dm.send(sock);

    TArray<struct FDirectoryItem> ret;

    // If the item count was 0, then we're really going to want to wait for a return message.
    if (dm.item_count == 0)
    {
        std::chrono::milliseconds dura(20);
        Diana::BSONMessage* m = NULL;
        for (int i = 0; ((i < 50) && (m == NULL)); i++)
        {
            m = Diana::BSONMessage::ReadMessage(sock);
            std::this_thread::sleep_for(dura);
        }

        if ((m != NULL) && (m->msg_type == Diana::BSONMessage::MessageType::Directory))
        {
            Diana::DirectoryMsg* dmr = (Diana::DirectoryMsg*)m;
            if ((dmr->server_id == server_id) && (dmr->client_id == client_id) &&
                (strcmp(TCHAR_TO_ANSI(*type), dmr->item_type) == 0))
            {
                struct FDirectoryItem item;
                for (int i = 0; i < dmr->item_count; i++)
                {
                    item.id = dmr->items[i].id;
                    item.name = FString(dmr->items[i].name);
                    ret.Add(item);
                }
            }
        }
    }

    return ret;
}

void ADianaConnector::CreateShip(int32 client_id, int32 server_id, int32 class_id)
{
    std::chrono::milliseconds dura(20);
    TArray<struct FDirectoryItem> selection;
    struct FDirectoryItem item;
    item.id = class_id;
    item.name = FString("");
    selection.Add(item);
    FString type = "CLASS";

    DirectoryListing(client_id, server_id, type, selection);
    Diana::BSONMessage* m = NULL;
    for (int i = 0; ((i < 50) && (m == NULL)); i++)
    {
        m = Diana::BSONMessage::ReadMessage(sock);
        std::this_thread::sleep_for(dura);
    }

    if (m->msg_type == Diana::BSONMessage::Hello)
    {
    }

    delete m;
}

int32 ADianaConnector::JoinShip(int32 client_id, int32 server_id, int32 ship_id)
{
    std::chrono::milliseconds dura(20);
    TArray<struct FDirectoryItem> selection;
    struct FDirectoryItem item;
    item.id = ship_id;
    item.name = FString("");
    selection.Add(item);
    FString type = "SHIP";
    int32 ret = -1;

    DirectoryListing(client_id, server_id, type, selection);
    Diana::BSONMessage* m = NULL;
    for (int i = 0; ((i < 50) && (m == NULL)); i++)
    {
        m = Diana::BSONMessage::ReadMessage(sock);
        std::this_thread::sleep_for(dura);
    }

    if (m->msg_type == Diana::BSONMessage::Hello)
    {
        ret = m->server_id;
    }

    delete m;
    return ret;
}

void ADianaConnector::RenameShip(int32 client_id, int32 server_id, FString new_name)
{
    Diana::NameMsg nm;
    nm.client_id = client_id;
    nm.server_id = server_id;
    nm.name = TCHAR_TO_ANSI(*new_name);
    nm.spec_all();
    nm.send(sock);
    nm.name = NULL;
}

void ADianaConnector::Ready(int32 client_id, int32 server_id)
{
    Diana::ReadyMsg rm;
    rm.client_id = client_id;
    rm.server_id = server_id;
    rm.ready = true;
    rm.spec_all();
    rm.send(sock);
}

void ADianaConnector::Goodbye(int32 client_id, int32 server_id)
{
    Diana::GoodbyeMsg gm;
    gm.server_id = server_id;
    gm.client_id = client_id;
    gm.spec_all();
    gm.send(sock);
}

void ADianaConnector::SetThrust(FVector _thrust)
{
    proxy->SetThrust(client_id, server_id, _thrust);
}

void ADianaConnector::SetThrust(int32 client_id, int32 server_id, FVector _thrust)
{
    thrust = _thrust;
    Diana::ThrustMsg tm;
    tm.client_id = client_id;
    tm.server_id = server_id;
    tm.thrust.x = thrust.X;
    tm.thrust.y = thrust.Y;
    tm.thrust.z = thrust.Z;
    tm.spec_all();
    tm.send(sock);
}

void ADianaConnector::OffsetThrust(FVector _thrust)
{
    proxy->OffsetThrust(client_id, server_id, _thrust);
}

void ADianaConnector::OffsetThrust(int32 client_id, int32 server_id, FVector _thrust)
{
    thrust += _thrust;
    Diana::ThrustMsg tm;
    tm.client_id = client_id;
    tm.server_id = server_id;
    tm.thrust.x = thrust.X;
    tm.thrust.y = thrust.Y;
    tm.thrust.z = thrust.Z;
    tm.spec_all();
    tm.send(sock);
}
