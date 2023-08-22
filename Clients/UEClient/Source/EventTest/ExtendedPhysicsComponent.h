// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "ExtendedPhysicsComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class EVENTTEST_API UExtendedPhysicsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UExtendedPhysicsComponent();

    // Called when the game starts
    virtual void BeginPlay() override;

    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        FVector GetAcceleration();

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        void SetAcceleration(FVector Acceleration);

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        FVector GetVelocity();

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        void SetVelocity(FVector Velocity);

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        FVector GetPosition();

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        void SetPosition(FVector Position);

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        void GetPVA(FVector& Position, FVector& Velocity, FVector& Acceleration);

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        void SetPVA(FVector Position, FVector Velocity, FVector Acceleration);

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        float GetLocalSimThreshold();

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        void SetLocalSimThreshold(float Threshold);

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        UStaticMeshComponent* GetStaticMesh();

    UFUNCTION(BlueprintCallable, Category = "Extended Physics")
        void SetStaticMesh(UStaticMeshComponent* StaticMesh);

    static void motion_interpolation(double* t, FVector* p, int32 order, FVector* components);

protected:
    // See: https://answers.unrealengine.com/questions/207675/fcriticalsection-lock-causes-crash.html
    FCriticalSection cs;

    bool local_physics_enabled = true;
    float local_physics_thresh = 0.05;
    UStaticMeshComponent* static_mesh = NULL;
    
    bool set_accel = false;
    FVector acceleration = FVector(0.0, 0.0, 0.0);
    bool set_velocity = false;
    FVector velocity = FVector(0.0, 0.0, 0.0);
    bool set_position = false;
    FVector position = FVector(0.0, 0.0, 0.0);
};
