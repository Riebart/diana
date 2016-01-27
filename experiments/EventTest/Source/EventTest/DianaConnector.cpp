// Fill out your copyright notice in the Description page of Project Settings.

#include "EventTest.h"
#include "DianaConnector.h"

#include "Array.h"

#include "Networking.h"
#include "Sockets.h"
#include "SocketTypes.h"
#include "SocketSubsystem.h"

#include "messaging.hpp"

#include <thread>

ADianaConnector::FVisDataReceiver::FVisDataReceiver(FSocket* sock, ADianaConnector* parent, std::map<int32, struct ADianaConnector::DianaActor>* oa_map)
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Constructor"));
    this->sock = sock;
    this->parent = parent;
    this->oa_map = oa_map;
    rt = FRunnableThread::Create(this, TEXT("VisDataReceiver"), 0, EThreadPriority::TPri_Normal); // Don't set an affinity mask
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Constructor::PostThreadCreate"));
}

ADianaConnector::FVisDataReceiver::~FVisDataReceiver()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Destructor"));
    Stop();
    delete rt;
    rt = NULL;
}

bool ADianaConnector::FVisDataReceiver::Init()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Init"));
    running = true;
    return true;
}

uint32 ADianaConnector::FVisDataReceiver::Run()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Run::Begin"));

    std::chrono::milliseconds dura(10);
    // Timespan representing 1 second as 10-million 0.1us (100ns) ticks.
    FTimespan sock_wait_time(10000000);
    bool read_available = false;
    Diana::BSONMessage* m = NULL;
    Diana::VisualDataMsg* vdm = NULL;
    struct DianaVDM dm;
    uint32 nmessages = 0;
    UWorld* world = parent->GetWorld();

    while (running)
    {
        read_available = sock->Wait(ESocketWaitConditions::Type::WaitForRead, sock_wait_time);
        if (read_available)
        {
            m = Diana::BSONMessage::BSONMessage::ReadMessage(sock);

            // Only consider messages that are VisualData, and have our client_id, and have the phys_id specced[2]
            if ((m != NULL) &&
                (m->msg_type == Diana::BSONMessage::VisualData) &&
                m->specced[1] && (m->client_id == parent->client_id))
            {
                // If the phys_id is specced, then use it.
                if (m->specced[2])
                {
                    vdm = (Diana::VisualDataMsg*)m;
                    dm.server_id = (int32)vdm->phys_id & 0x7FFFFFFF; // Unsign the ID, because that's natural.
                    dm.world_time = world->RealTimeSeconds;
                    dm.pos = FVector(vdm->position.x, vdm->position.y, vdm->position.z);
                    parent->messages.Enqueue(dm);

                    // We handled a message, so immediately repeat, skip the Sleep()
                    delete vdm;
                }
                // If the phys_id is unspecced, then we need to check up on our map, and see
                // which objects missed their update, indicating they should be removed from
                // the scene.
                else
                {
                    // Push an end-of-frame VDM to the queue
                    dm.server_id = 0x80000000;
                    dm.world_time = world->RealTimeSeconds;
                    parent->messages.Enqueue(dm);
                }

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

void ADianaConnector::FVisDataReceiver::Stop()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataRecvThread::Stop"));
    running = false;
    rt->WaitForCompletion();
}

bool ADianaConnector::ConnectSocket()
{
    if (sock == NULL)
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
        return true;
    }
}

void ADianaConnector::DisconnectSocket()
{
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::DisconnectSocket"));
    if (sock != NULL)
    {
        sock->Close();
    }
}

// Sets default values
ADianaConnector::ADianaConnector()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

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
    struct DianaActor da;

    while (!messages.IsEmpty())
    {
        messages.Dequeue(dm);

        if (dm.server_id == 0x80000000)
        {
            vis_iteration++;
            continue;
        }

        da.server_id = dm.server_id;

        // Look up the server ID in the map
        it = oa_map.find(dm.server_id);
        if (it == oa_map.end())
        {
            // The object is new
            //UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::Tick::HandleNew"));
            da.last_time = dm.world_time;
            da.cur_time = dm.world_time;
            da.a = NULL;
            da.last_pos = dm.pos;
            da.cur_pos = da.last_pos;
            da.last_iteration = vis_iteration;
            {
                FScopeLock Lock(&map_cs);
                oa_map[da.server_id] = da;
            }
            NewVisDataObject(da.server_id, da.cur_pos);
        }
        else
        {
            // The object is being updated
            //UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::Tick::HandleUpdated"));
            da = oa_map[da.server_id];
            da.last_time = da.cur_time;
            da.cur_time = dm.world_time;
            da.last_pos = da.cur_pos;
            da.cur_pos = dm.pos;
            da.last_iteration = vis_iteration;
            if (da.a != NULL)
            {
                //da.a->SetActorLocation(da.cur_pos);
                oa_map[da.server_id] = da;
            }
            ExistingVisDataObject(da.server_id, da.cur_pos, da.last_pos, da.cur_time, da.last_time, da.a);
        }
    }

    for (std::map<int32, struct DianaActor>::iterator it = oa_map.begin(); it != oa_map.end(); )
    {
        da = it->second;
        // We may have only consumed part of the current frame, so it it proper to check that
        // the last iteration value is at least onemore than the previous value.
        if (da.last_iteration < (vis_iteration - 1))
        {
            it = oa_map.erase(it);
            RemovedVisDataObject(da.server_id, da.a);
        }
        else
        {
            it++;
        }
    }
}

bool ADianaConnector::RegisterForVisData(bool enable)
{
    return RegisterForVisData(enable, client_id, server_id);
}

bool ADianaConnector::RegisterForVisData(bool enable, int32 client_id, int32 server_id)
{
    // If we're disconnected (NULL worker thread), and trying to disconnect again, just do nothing.
    if ((vdr_thread == NULL) && !enable)
    {
        return true;
    }

    ConnectSocket();

    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::VisDataToggle::Begin"));
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

void ADianaConnector::UpdateExistingVisDataObject(int32 PhysID, AActor* ActorRef)
{
    FScopeLock Lock(&map_cs);
    oa_map[PhysID].a = ActorRef;
}

TArray<struct FDirectoryItem> ADianaConnector::DirectoryListing(FString type, TArray<struct FDirectoryItem> items)
{
    return DirectoryListing(client_id, server_id, type, items);
}

void ADianaConnector::CreateShip(int32 class_id)
{
    CreateShip(client_id, server_id, class_id);
}

void ADianaConnector::JoinShip()
{
    JoinShip(client_id, server_id);
}

void ADianaConnector::RenameShip(FString NewShipName)
{
    RenameShip(client_id, server_id, NewShipName);
}

void ADianaConnector::Ready()
{
    Ready(client_id, server_id);
}

void ADianaConnector::Goodbye()
{
    Goodbye(client_id, server_id);
}

TArray<struct FDirectoryItem> ADianaConnector::DirectoryListing(int32 client_id, int32 server_id, FString type, TArray<struct FDirectoryItem> items)
{
    TArray<struct FDirectoryItem> ret;
    struct FDirectoryItem i;
    i.name = FString("2560x1440");
    i.id = 1;
    ret.Add(i);
    i.name = FString("1920x1080");
    i.id = 2;
    ret.Add(i);
    i.name = FString("1280x720");
    i.id = 3;
    ret.Add(i);
    return ret;
}

void ADianaConnector::CreateShip(int32 client_id, int32 server_id, int32 class_id)
{
}

void ADianaConnector::JoinShip(int32 client_id, int32 server_id)
{
}

void ADianaConnector::RenameShip(int32 client_id, int32 server_id, FString NewShipName)
{
}

void ADianaConnector::Ready(int32 client_id, int32 server_id)
{
}

void ADianaConnector::Goodbye(int32 client_id, int32 server_id)
{
}
