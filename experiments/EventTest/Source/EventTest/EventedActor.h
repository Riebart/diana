// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <map>
#include "GameFramework/Actor.h"
#include "EventedActor.generated.h"

class FSocket;
class AEventedActor;

struct DianaActor
{
    int32 server_id;
    AActor* a;
    float last_time;
    float cur_time;
    FVector last_pos;
    FVector cur_pos;
};

// See: https://wiki.unrealengine.com/Multi-Threading:_How_to_Create_Threads_in_UE4
class FVisDataReceiver : public FRunnable
{
public:
    FVisDataReceiver(FSocket* sock, AEventedActor* parent, std::map<int32, struct DianaActor>* oa_map);
    ~FVisDataReceiver();

    virtual bool Init();
    virtual uint32 Run();
    virtual void Stop();

private:
    FRunnableThread* rt = NULL;
    volatile bool running;
    FSocket* sock = NULL;
    AEventedActor* parent;
    std::map<int32, struct DianaActor>* oa_map;
};

UCLASS()
class EVENTTEST_API AEventedActor : public AActor
{
    friend class FVisDataReceiver;
	
    GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
    AEventedActor();
    ~AEventedActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server information", meta = (DisplayName = "Server IPv4 Address"))
        FString host = "127.0.0.1";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server information", meta = (DisplayName = "Server TCP Port"))
        int32 port = 5505;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server information", meta = (DisplayName = "Client ID"))
        int32 client_id = -1;

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        bool RegisterForVisData(bool enable);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void UpdateExistingVisDataObject(int32 PhysID, AActor* ActorRef);

    // Don't have access to doubles, or 64-bit ints in Blueprints.
    // See: https://answers.unrealengine.com/questions/98206/missing-support-for-uint32-int64-uint64.html
    UFUNCTION(BlueprintImplementableEvent, Category = "Messages From Diana", meta = (DisplayName = "Received Vis Data Message for New Object"))
        void NewVisDataObject(int32 PhysID, FVector Position);
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Messages From Diana", meta = (DisplayName = "Received Vis Data Message for Existing Object"))
        void ExistingVisDataObject(int32 PhysID, FVector CurrentPosition, FVector LastPosition, float CurrentRealTime, float LastRealTime, AActor* ActorRef);

private:
    FVisDataReceiver* vdr_thread = NULL;
    bool ConnectSocket();
    void DisconnectSocket();
    FSocket* sock = NULL;

    // See: http://www.slideserve.com/maine/concurrency-parallelism-in-ue4
    TQueue<struct DianaActor> new_objects;
    TQueue<struct DianaActor> updated_objects;
    std::map<int32, struct DianaActor> oa_map;
};
