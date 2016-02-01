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

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        FVector GetAcceleration();

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void SetAcceleration(FVector Acceleration);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        FVector GetVelocity();

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void SetVelocity(FVector Velocity);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        FVector GetPosition();

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void SetPosition(FVector Position);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void GetPVA(FVector& Position, FVector& Velocity, FVector& Acceleration);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void SetPVA(FVector& Position, FVector& Velocity, FVector& Acceleration);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        UStaticMeshComponent* GetStaticMesh();

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void SetStaticMesh(UStaticMeshComponent* StaticMesh);

protected:
    // See: https://answers.unrealengine.com/questions/207675/fcriticalsection-lock-causes-crash.html
    FCriticalSection cs;

    UStaticMeshComponent* static_mesh = NULL;
    
    FVector acceleration = FVector(0.0, 0.0, 0.0);
    FVector velocity = FVector(0.0, 0.0, 0.0);
    FVector position = FVector(0.0, 0.0, 0.0);
};
