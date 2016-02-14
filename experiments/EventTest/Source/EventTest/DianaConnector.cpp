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
#include <chrono>

#define ALMOST_ZERO(v) (((v) >= -1e-8) || ((v) <= 1e-8))

FVector FVzero = FVector(0.0, 0.0, 0.0);

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
    ADianaConnector* parent = NULL;
    std::map<int32, struct ADianaConnector::DianaActor*>* oa_map = NULL;
};

class FSensorManager : public FRunnable
{
public:
    FSensorManager(int32 system_id, ADianaConnector* parent, std::map<FString, struct FSensorContact>* sc_map);
    ~FSensorManager();

    virtual bool Init();
    virtual uint32 Run();
    virtual void Stop();
    volatile bool paused;

private:
    FRunnableThread* rt = NULL;
    volatile bool running;
    int32 system_id;
    ADianaConnector* parent = NULL;
    std::map<FString, struct FSensorContact>* sc_map = NULL;
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

FSensorManager::FSensorManager(int32 system_id, ADianaConnector* parent, std::map<FString, struct FSensorContact>*sc_map)
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::FSensorManager::Constructor"));
    this->system_id = system_id;
    this->parent = parent;
    this->sc_map = sc_map;
    rt = FRunnableThread::Create(this, TEXT("FSensorManager"), 0, EThreadPriority::TPri_Normal); // Don't set an affinity mask
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::FSensorManager::Constructor::PostThreadCreate"));
}

FVisDataReceiver::~FVisDataReceiver()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Destructor"));
    Stop();
    delete rt;
    rt = NULL;
}

FSensorManager::~FSensorManager()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::FSensorManager::Destructor"));
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

bool FSensorManager::Init()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::FSensorManager::Init"));
    running = true;
    paused = true;
    return true;
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
    FVector motion_components[2];
    uint32 nmessages = 0;
    UWorld* world = parent->GetWorld();
    float last_stat_time = world->RealTimeSeconds;
    int last_stat_frames = 0;

    // Hold a list of all DianaActor struct pointers, we're going to use it like a stack
    // to make it easier to expire objects at the end of a vis frame.
    std::list<struct ADianaConnector::DianaActor*> last_seen;
    std::list<struct ADianaConnector::DianaActor*>::iterator lit;
    int64 vis_iterations_buffer = 5;
    int64 vis_iterations = vis_iterations_buffer;

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
            {
                FScopeLock(&parent->bson_cs);
                m = Diana::BSONMessage::BSONMessage::ReadMessage(sock);
            }

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
                    // Why am I not scaling the radius?
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
                        parent->vis_messages.Enqueue(dm);

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
                            UExtendedPhysicsComponent::motion_interpolation(da->time, da->pos, 2, motion_components);
                            da->epc->SetPVA(da->pos[2], motion_components[0], motion_components[1]);
                        }
                    }
                }
                // If the phys_id is unspecced, then we need to check up on our list, and see
                // which objects missed their update, indicating they should be removed from
                // the scene.
                else
                {
                    for (lit = last_seen.begin(); ((lit != last_seen.end()) && ((*lit)->last_iteration < (vis_iterations - vis_iterations_buffer))); )
                    {
                        // In this case, just set the da pointer to be to the object that
                        // is expired, the non-NULL-ness will trigger the removal, and the
                        // rest of the dm struct will be ignored.
                        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Erasing %d"), (*lit)->server_id);
                        dm.da = *lit;
                        dm.server_id = (*lit)->server_id;
                        lit = last_seen.erase(lit);
                        parent->vis_messages.Enqueue(dm);
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

uint32 FSensorManager::Run()
{
    uint32 ncontacts = 0;
    uint32 cur_contacts;
    while (running)
    {
        std::chrono::milliseconds dura(10);
        if (!paused)
        {
            cur_contacts = parent->SensorStatus(true, system_id);
        }
        else
        {
            cur_contacts = 0;
        }
        ncontacts += cur_contacts;
        if (cur_contacts == 0)
        {
            std::this_thread::sleep_for(dura);
        }
    }
    return ncontacts;
}

void FVisDataReceiver::Stop()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Stop"));
    running = false;
    rt->WaitForCompletion();
}

void FSensorManager::Stop()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::FSensorManager::Stop"));
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
    if (sensor_thread != NULL)
    {
        sensor_thread->Stop();
        delete sensor_thread;
        sensor_thread = NULL;
    }
    DisconnectSocket();
}

// Called when the game starts or when spawned
void ADianaConnector::BeginPlay()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging:BeginPlay"));
    Super::BeginPlay();
    world_to_metres = this->GetWorld()->GetWorldSettings()->WorldToMeters / 2.0;;
}

// Called every frame
void ADianaConnector::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    struct DianaVDM dm;
    struct DianaActor* da;

    while (!vis_messages.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::Tick::HandlingQueue::VisMessage"));
        vis_messages.Dequeue(dm);

        // If the object disappeared, call out to Blueprints to handle that event
        if (dm.da != NULL)
        {
            da = dm.da;

            // Sometimes, if the vis frames are fast enough, and the objects are moving, the vis frames will
            // arrive in an order that causes an object to be missed in one or more frames, possibly
            // before the Blueprint returns with an actor allocation.
            if (da->a != NULL)
            {
                RemovedVisDataObject(da->server_id, da->a, da->epc);
            }
            delete dm.da;
            FScopeLock(&this->map_cs);
            oa_map.erase(dm.server_id);
        }
        // Otherweise call out to Blueprints to handle a new object.
        else
        {
            // The object is new
            NewVisDataObject(dm.server_id, dm.radius, dm.pos);
        }
    }

    FString contact_id;
    std::map<FString, struct FSensorContact>::iterator it;
    struct FSensorContact* sc;
    while (!sensor_messages.IsEmpty())
    {
        sensor_messages.Dequeue(contact_id);
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::Tick::HandlingQueue::SensorMessage %s"), *contact_id);
        {
            FScopeLock(&this->map_cs);
            it = sc_map.find(contact_id);
        }

        if (it != sc_map.end())
        {
            sc = &(it->second);
            SensorContact(it->second, sc->actor, sc->epc);
        }
        // If it was in the queue, but we can't find it, it was likely removed almost immediately after
        // being added.
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::Tick::HandlingQueue::SensorMessage::NotInMap %s"), *contact_id);
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

            FScopeLock Lock(&map_cs);
            struct DianaActor* da;
            std::map<int32, struct DianaActor*>::iterator it;
            for (it = oa_map.begin(); it != oa_map.end(); it++)
            {
                da = it->second;
                RemovedVisDataObject(da->server_id, da->a, da->epc);
                delete da;
            }
            oa_map.clear();
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

int32 ADianaConnector::SensorStatus(bool read_only, int32 system_id)
{
    return proxy->SensorStatus(client_id, server_id, read_only, system_id);
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

    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::DirectoryListing %u"), dm.item_count);

    // If the item count was 0, then we're really going to want to wait for a return message.
    if (dm.item_count == 0)
    {
        std::chrono::milliseconds dura(20);
        Diana::BSONMessage* m = NULL;
        for (int i = 0; ((i < 50) && (m == NULL)); i++)
        {
            {
                FScopeLock(&this->bson_cs);
                m = Diana::BSONMessage::ReadMessage(sock);
            }
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
        {
            FScopeLock(&this->bson_cs);
            m = Diana::BSONMessage::ReadMessage(sock);
        }
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
        {
            FScopeLock(&this->bson_cs);
            m = Diana::BSONMessage::ReadMessage(sock);
        }
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

#define CHILD_MAP(var, map, key) var = (map)->at(key).map_val; if ((var) == NULL) { throw new std::out_of_range("NULL child map"); }

struct FSensorContact read_contact(std::map<std::string, struct BSONReader::Element>* map)
{
    struct FSensorContact v;
    char* str;
    std::map<std::string, struct BSONReader::Element>* tmp;

    try
    {
        CHILD_MAP(tmp, map, "position");
        v.position = FVector(tmp->at("x").dbl_val, tmp->at("y").dbl_val, tmp->at("z").dbl_val);
        CHILD_MAP(tmp, map, "velocity");
        v.velocity = FVector(tmp->at("x").dbl_val, tmp->at("y").dbl_val, tmp->at("z").dbl_val);
        CHILD_MAP(tmp, map, "thrust");
        v.thrust = FVector(tmp->at("x").dbl_val, tmp->at("y").dbl_val, tmp->at("z").dbl_val);
        CHILD_MAP(tmp, map, "orientation");
        v.forward = FVector(tmp->at("w").dbl_val, tmp->at("x").dbl_val, 0.0f);
        v.forward.Z = 1 - v.forward.Size();
        v.up = FVector(tmp->at("y").dbl_val, tmp->at("z").dbl_val, 0.0f);
        v.up.Z = 1 - v.up.Size();
        v.right = FVector::CrossProduct(v.forward, v.up);
        v.mass = map->at("mass").dbl_val;
        v.radius = map->at("radius").dbl_val;
        v.time_seen = map->at("time_seen").dbl_val;

        str = map->at("name").str_val;
        v.name = (str != NULL ? str : "");
        str = map->at("data").str_val;
        v.data = (str != NULL ? str : "");
    }
    catch (std::out_of_range e)
    {
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::read_contact Missing critial key or NULL sub-map"));
    }

    return v;
}

struct FSensorSystem read_scanner(std::map<std::string, struct BSONReader::Element>* map)
{
    struct FSensorSystem v;
    char* str;

    try
    {
        v.horizontal_focus_min = map->at("h_min").dbl_val;
        v.horizontal_focus_max = map->at("h_max").dbl_val;
        v.vertical_focus_min = map->at("v_min").dbl_val;
        v.vertical_focus_max = map->at("v_max").dbl_val;
        v.max_energy = map->at("max_power").dbl_val;
        v.bank_id = map->at("bank_id").i64_val;
        v.time_last_fired = map->at("time_fired").dbl_val;
        v.recharge_time = map->at("recharge_time").dbl_val;
        v.damage_level = map->at("damage_level").dbl_val;

        str = map->at("type").str_val;
        v.type = (str != NULL ? str : "");
    }
    catch (std::out_of_range e)
    {
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::read_contact Missing critial key or NULL sub-map"));
    }

    return v;
}

int32 ADianaConnector::SensorStatus(int32 client_id, int32 server_id, bool read_only, int32_t system_id)
{
    FScopeLock(&this->sensor_cs);
    ConnectSocket();

    if (sensor_thread == NULL)
    {
        sensor_thread = new FSensorManager(system_id, this, &sc_map);
        sensor_thread->paused = false;
    }

    if (!read_only)
    {
        Diana::CommandMsg cm;
        cm.client_id = client_id;
        cm.server_id = server_id;
        cm.system_id = system_id;
        cm.command = new BSONReader::Element();
        cm.command->type = BSONReader::ElementType::String;
        cm.command->str_val = "blah"; // new char[] {"blah"};
        cm.command->str_len = 4;
        cm.command->name = "\x04\x00";
        cm.spec_all();
        cm.send(sock);
        cm.command->name = NULL;
        cm.command->str_val = NULL;
        return 0;
    }

    int32 ncontacts = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    start = std::chrono::high_resolution_clock::now();

    Diana::BSONMessage* m = NULL;;
    //std::chrono::milliseconds dura(20);
    //std::this_thread::sleep_for(dura);
    {
        FScopeLock(&this->bson_cs);
        m = Diana::BSONMessage::BSONMessage::ReadMessage(sock);
    }

    //std::chrono::milliseconds dura(20);
    //for (int i = 0; ((i < 50) && (m == NULL)); i++)
    //{
    //    m = Diana::BSONMessage::BSONMessage::ReadMessage(sock);
    //    std::this_thread::sleep_for(dura);
    //}

    if ((m != NULL) &&
        (m->msg_type == Diana::BSONMessage::MessageType::SystemUpdate) &&
        (m->specced[0] && m->specced[1] && m->specced[2]) &&
        (m->client_id == client_id) &&
        (m->server_id == m->server_id))
    {
        Diana::SystemUpdateMsg* msg = (Diana::SystemUpdateMsg*)m;
        if (msg->properties->map_val == NULL)
        {
            return ncontacts;
        }

        //SEND OrderedDict([('', 23), ('\x01', 3), ('\x02', 1L), 
        //('\x03', { 
        //'fade_time': 5.0, 
        //'scanners' : [{'v_max': 6.283185307179586, 
        //               'h_max' : 6.283185307179586, 
        //               'v_min' : 0.0, 
        //               'h_min' : 0.0, 
        //               'max_power' : 10000, 
        //               'bank_id' : 0, 
        //               'type' : 'Scan Emitter', 
        //               'time_fired' : 1455164838.601, 
        //               'recharge_time' : 2.0, 
        //               'damage_level' : 0.0}], 
        //'contacts' : {
        //        u'NonRadiatorDumb[(9.0, 9.08418508866189)]': {
        //                u'data': None,
        //                u'mass' : 1.0,
        //                u'name' : u'NonRadiatorDumb',
        //                u'orientation' : {
        //                    u'w': 1.0,
        //                    u'x' : 0.0,
        //                    u'y' : 0.0,
        //                    u'z' : 0.0},
        //                u'position' : {
        //                    u'x': 106.4448228912682,
        //                    u'y' : 226.20676311650487,
        //                    u'z' : 0.0},
        //                u'rad_sig' : {
        //                    u'spectrum': [
        //                        [9.0, 9.08418508866189]
        //                    ]},
        //                u'radius' : 1.0,
        //                u'thrust' : {
        //                    u'x': 0.0,
        //                    u'y' : 0.0,
        //                    u'z' : 0.0},
        //                u'time_seen' : 1455167101.759,
        //                u'velocity' : {
        //                    u'x': 0.0,
        //                    u'y' : 0.0,
        //                    u'z' : 0.0}
        //            }},
        //'name' : 'Sensors', 
        //'controlled' : 1, 
        //'system_id' : -1, 
        //'_Observable__osim_id' : 3 })])

        std::map<std::string, struct BSONReader::Element>* props = msg->properties->map_val;

        double fade_time = props->at("fade_time").dbl_val;
        FString system_name = props->at("name").str_val;

        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SensorStatus::FadeTime %f"), fade_time);

        std::map<std::string, struct BSONReader::Element>::iterator it;

        //TArray<struct FSensorContact> contacts;
        try
        {
            struct BSONReader::Element* el = &(props->at("contacts"));
            if (el->map_val != NULL)
            {
                FVector position;
                FVector velocity;
                FVector acceleration;

                for (it = el->map_val->begin(); it != el->map_val->end(); it++)
                {
                    if (it->second.map_val != NULL)
                    {
                        struct FSensorContact v = read_contact(it->second.map_val);
                        v.contact_id = it->first.c_str();

                        std::map<FString, struct FSensorContact>::iterator scit;
                        FScopeLock(&this->map_cs);
                        scit = sc_map.find(v.contact_id);
                        ncontacts++;

                        // If we don't currently have an entry for this contact
                        // None of this involves updating the epc components.
                        if (scit == sc_map.end())
                        {
                            sc_map[v.contact_id] = v;
                            v.actor = NULL;
                            v.epc = NULL;
                            sensor_messages.Enqueue(v.contact_id);
                            //SensorContact(v, NULL, NULL);
                            UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SensorStatus::NewContact %s"), *v.contact_id);
                        }
                        else if (v.time_seen > scit->second.time_seen)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SensorStatus::UpdatedContact %s"), *v.contact_id);
                            v.actor = scit->second.actor;
                            v.epc = scit->second.epc;
                            if (v.epc != NULL)
                            {
                                position = world_to_metres * v.position;
                                velocity = world_to_metres * v.velocity;
                                acceleration = world_to_metres * (ALMOST_ZERO(v.mass) ? FVzero : v.thrust / v.mass);
                                v.epc->SetPVA(position, velocity, acceleration);
                            }
                            
                            scit->second = v;
                            sensor_messages.Enqueue(v.contact_id);
                        }
                    }
                }
            }
        }
        catch (std::out_of_range e)
        {
            UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SensorStatus Expected 'contacts' element at root, not found."));
        }

        try
        {
            struct BSONReader::Element* el = &(props->at("scanners"));
            if (el->map_val != NULL)
            {
                for (it = el->map_val->begin(); it != el->map_val->end(); it++)
                {
                    if (it->second.map_val != NULL)
                    {
                        struct FSensorSystem v = read_scanner(it->second.map_val);
                        v.fade_time = fade_time;
                    }
                }
            }
        }
        catch (std::out_of_range e)
        {
            UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SensorStatus Expected 'scanners' element at root, not found."));
        }

        delete msg;
    }
    else
    {
        delete m;
    }

    end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed;
    elapsed = end - start;
    double e = elapsed.count();
    if (e > 1e-4)
    {
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SensorStatus::MessageHandleTime %g"), e);
    }

    return ncontacts;
}

void ADianaConnector::UpdateExistingSensorContact(const FString& ID, AActor* ActorRef, UExtendedPhysicsComponent* EPCRef)
{
    if ((ActorRef != NULL) && (EPCRef != NULL))
    {
        FScopeLock(&this->map_cs);
        struct FSensorContact c;
        c = sc_map[ID];
        c.actor = ActorRef;
        c.epc = EPCRef;

        float world_to_metres = this->GetWorld()->GetWorldSettings()->WorldToMeters / 2.0;
        FVector position = world_to_metres * c.position;
        FVector velocity = world_to_metres * c.velocity;
        FVector acceleration = world_to_metres * (ALMOST_ZERO(c.mass) ? FVzero : c.thrust / c.mass);
        EPCRef->SetPVA(position, velocity, acceleration);
        sc_map[ID] = c;
    }
    else
    {
        //FScopeLock(&this->map_cs);
        //sc_map.erase(ID);
    }
}
