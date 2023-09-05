// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <map>

#include "ExtendedPhysicsComponent.h"

#include "GameFramework/Actor.h"
#include "DianaConnector.generated.h"

class FSocket;

UENUM(BlueprintType)
enum class FCoordinateTransformFunction : uint8
{
    Linear,
    Sqrt,
    Log2,
    Log10
};

USTRUCT(BlueprintType, Blueprintable)
struct FVisScalingBehaviour
{
    GENERATED_USTRUCT_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization Scaling")
    FCoordinateTransformFunction coordinate_scaling_func;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization Scaling")
    FCoordinateTransformFunction object_scaling_func;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization Scaling")
    float coordinate_scale_factor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization Scaling")
    float object_scale_factor;
};

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    FVector position;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    FVector velocity;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    FVector thrust;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    FVector forward;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    FVector up;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    FVector right;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    float mass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    float radius;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    float time_seen;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    FString name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    FString data;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    FString contact_id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    AActor *actor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor Contact")
    UExtendedPhysicsComponent *epc;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSensorSystem
{
    GENERATED_USTRUCT_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    float fade_time;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    float horizontal_focus_min;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    float horizontal_focus_max;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    float vertical_focus_min;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    float vertical_focus_max;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    float max_energy;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    int32 bank_id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    FString type;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    float time_last_fired;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    float recharge_time;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sensor System")
    float damage_level;
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
        AActor *a;
        UExtendedPhysicsComponent *epc;
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
        struct DianaActor *da;
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

    // UPROPERTY(BlueprintReadWrite, Category = "Server information", meta = (DisplayName = "Visualization World Coordinate Scale"))
    // float vis_world_coordinate_scale = 1.0f;

    // UPROPERTY(BlueprintReadWrite, Category = "Server information", meta = (DisplayName = "Visualization World Object Scale"))
    // float vis_world_object_scale = 1.0f;

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
    bool ConnectToServer(FString _host, int32 _port);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
    void UseProxyConnection(ADianaConnector *_proxy);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
    bool RegisterForVisData(bool enable, struct FVisScalingBehaviour vsb);

    // Don't have access to doubles, or 64-bit ints in Blueprints.
    // See: https://answers.unrealengine.com/questions/98206/missing-support-for-uint32-int64-uint64.html
    UFUNCTION(BlueprintImplementableEvent, Category = "Messages From Diana", meta = (DisplayName = "New Vis Data Object"))
    void NewVisDataObject(int32 PhysID, float Radius, FVector Position);

    UFUNCTION(BlueprintImplementableEvent, Category = "Messages From Diana", meta = (DisplayName = "Removed Vis Data Object"))
    void RemovedVisDataObject(int32 PhysID, AActor *ActorRef, UExtendedPhysicsComponent *EPCRef);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
    void UpdateExistingVisDataObject(int32 PhysID, AActor *ActorRef, UExtendedPhysicsComponent *EPCRef);

    UFUNCTION(BlueprintImplementableEvent, Category = "Messages From Diana", meta = (DisplayName = "Received Sensor Contact"))
    void SensorContact(struct FSensorContact Contact, AActor *ActorRef, UExtendedPhysicsComponent *EPCRef);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
    void UpdateExistingSensorContact(const FString &ContactID, AActor *ActorRef, UExtendedPhysicsComponent *EPCRef);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
    TArray<struct FDirectoryItem> DirectoryListing(FString type, TArray<struct FDirectoryItem> items);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
    int32 SensorStatus(bool read_only, int32 system_id);

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
    bool RegisterForVisData(bool enable, int32 client_id, int32 server_id, struct FVisScalingBehaviour vsb);
    TArray<struct FDirectoryItem> DirectoryListing(int32 client_id, int32 server_id, FString type, TArray<struct FDirectoryItem> items);
    void CreateShip(int32 client_id, int32 server_id, int32 class_id);
    int32 JoinShip(int32 client_id, int32 server_id, int32 ship_id);
    void RenameShip(int32 client_id, int32 server_id, FString new_name);
    void Ready(int32 client_id, int32 server_id);
    void Goodbye(int32 client_id, int32 server_id);
    void SetThrust(int32 client_id, int32 server_id, FVector _thrust);
    void OffsetThrust(int32 client_id, int32 server_id, FVector _thrust);
    int32 SensorStatus(int32 client_id, int32 server_id, bool read_only, int32 system_id);

private:
    FString host = "";
    int32 port = 0;
    int64 vis_iteration = 1;
    ADianaConnector *proxy = NULL;
    FVisDataReceiver *vdr_thread = NULL;
    FSensorManager *sensor_thread = NULL;
    bool ConnectSocket();
    void DisconnectSocket();
    FSocket *sock = NULL;
    FVector thrust = FVector(0.0, 0.0, 0.0);
    float world_to_metres;

    // See: http://www.slideserve.com/maine/concurrency-parallelism-in-ue4
    TQueue<struct DianaVDM> vis_messages;
    TQueue<FString> sensor_messages;

    // See: https://answers.unrealengine.com/questions/207675/fcriticalsection-lock-causes-crash.html
    FCriticalSection map_cs;
    FCriticalSection bson_cs;
    FCriticalSection sensor_cs;

    std::map<int32, struct DianaActor *> oa_map;
    std::map<FString, struct FSensorContact> sc_map;
};
