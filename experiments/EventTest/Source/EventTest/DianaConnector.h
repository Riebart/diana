// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <map>

#include "ExtendedPhysicsComponent.h"

#include "GameFramework/Actor.h"
#include "DianaConnector.generated.h"

class FSocket;

USTRUCT(BlueprintType, Blueprintable)
struct FDirectoryItem
{
    GENERATED_USTRUCT_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Diana Messaging")
        FString name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Diana Messaging")
        int32 id;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSensorContact
{
    GENERATED_USTRUCT_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Diana Messaging")
        FString name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Diana Messaging")
        int32 id;
};

class FVisDataReceiver;
class FSensorManager;

UCLASS()
class EVENTTEST_API ADianaConnector : public AActor
{
    friend class FVisDataReceiver;
    friend class FSensorManager;

    GENERATED_BODY()

public:
    struct DianaActor
    {
        int32 server_id;
        AActor* a;
        UExtendedPhysicsComponent* epc;
        double radius;
        double time[3];
        FVector pos[3];
        int64 last_iteration;
    };

    struct DianaVDM
    {
        int32 server_id;
        float world_time;
        double radius;
        FVector pos;
        struct DianaActor* da;
    };

    // Sets default values for this actor's properties
    ADianaConnector();
    ~ADianaConnector();

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Called every frame
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadWrite, Category = "Server information", meta = (DisplayName = "Client ID"))
        int32 client_id = 1;

    UPROPERTY(BlueprintReadWrite, Category = "Server information", meta = (DisplayName = "Server ID"))
        int32 server_id = -1;

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        bool ConnectToServer(FString _host, int32 _port);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void UseProxyConnection(ADianaConnector* _proxy);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        bool RegisterForVisData(bool enable);

    // Don't have access to doubles, or 64-bit ints in Blueprints.
    // See: https://answers.unrealengine.com/questions/98206/missing-support-for-uint32-int64-uint64.html
    UFUNCTION(BlueprintImplementableEvent, Category = "Messages From Diana", meta = (DisplayName = "New Vis Data Object"))
        void NewVisDataObject(int32 PhysID, float Radius, FVector Position);

    UFUNCTION(BlueprintImplementableEvent, Category = "Messages From Diana", meta = (DisplayName = "Removed Vis Data Object"))
        void RemovedVisDataObject(int32 PhysID, AActor* ActorRef, UExtendedPhysicsComponent* EPCRef);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void UpdateExistingVisDataObject(int32 PhysID, AActor* ActorRef, UExtendedPhysicsComponent* EPCRef);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        bool ConnectToSystem(bool enable, int32 system_id);

    UFUNCTION(BlueprintImplementableEvent, Category = "Messages From Diana", meta = (DisplayName = "Received Sensor Contact"))
        void SensorContact(const FString& ContactID, AActor* ActorRef, UExtendedPhysicsComponent* EPCRef);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void UpdateExistingSensorContact(const FString& ContactID, AActor* ActorRef, UExtendedPhysicsComponent* EPCRef);
    
    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        TArray<struct FDirectoryItem> DirectoryListing(FString type, TArray<struct FDirectoryItem> items);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void CreateShip(int32 class_id);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        int32 JoinShip(int32 ship_id);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void RenameShip(FString new_name);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void Ready();

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void Goodbye();

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void SetThrust(FVector _thrust);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void OffsetThrust(FVector _thrust);

protected:
    bool RegisterForVisData(bool enable, int32 client_id, int32 server_id);
    TArray<struct FDirectoryItem> DirectoryListing(int32 client_id, int32 server_id, FString type, TArray<struct FDirectoryItem> items);
    void CreateShip(int32 client_id, int32 server_id, int32 class_id);
    int32 JoinShip(int32 client_id, int32 server_id, int32 ship_id);
    void RenameShip(int32 client_id, int32 server_id, FString new_name);
    void Ready(int32 client_id, int32 server_id);
    void Goodbye(int32 client_id, int32 server_id);
    void SetThrust(int32 client_id, int32 server_id, FVector _thrust);
    void OffsetThrust(int32 client_id, int32 server_id, FVector _thrust);

private:
    FString host = "";
    int32 port = 0;
    int64 vis_iteration = 1;
    ADianaConnector* proxy = NULL;
    FVisDataReceiver* vdr_thread = NULL;
    bool ConnectSocket();
    void DisconnectSocket();
    FSocket* sock = NULL;
    FVector thrust = FVector(0.0, 0.0, 0.0);

    // See: http://www.slideserve.com/maine/concurrency-parallelism-in-ue4
    TQueue<struct DianaVDM> messages;

    // See: https://answers.unrealengine.com/questions/207675/fcriticalsection-lock-causes-crash.html
    FCriticalSection map_cs;

    std::map<int32, struct DianaActor*> oa_map;
};
