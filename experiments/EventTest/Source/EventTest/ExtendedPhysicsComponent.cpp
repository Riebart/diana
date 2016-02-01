// Fill out your copyright notice in the Description page of Project Settings.

#include "EventTest.h"
#include "ExtendedPhysicsComponent.h"


// Sets default values for this component's properties
UExtendedPhysicsComponent::UExtendedPhysicsComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    bWantsBeginPlay = true;
    PrimaryComponentTick.bCanEverTick = true;

    // ...
}


// Called when the game starts
void UExtendedPhysicsComponent::BeginPlay()
{
    Super::BeginPlay();

    // ...

}

FVector UExtendedPhysicsComponent::GetAcceleration()
{
    FScopeLock(&this->cs);
    return this->acceleration;
}

void UExtendedPhysicsComponent::SetAcceleration(FVector Acceleration)
{
    FScopeLock(&this->cs);
    this->acceleration = Acceleration;
}

FVector UExtendedPhysicsComponent::GetVelocity()
{
    FScopeLock(&this->cs);
    return this->velocity;
}

void UExtendedPhysicsComponent::SetVelocity(FVector Velocity)
{
    FScopeLock(&this->cs);
    this->velocity = Velocity;
}

FVector UExtendedPhysicsComponent::GetPosition()
{
    FScopeLock(&this->cs);
    return this->position;
}

void UExtendedPhysicsComponent::SetPosition(FVector Position)
{
    FScopeLock(&this->cs);
    this->position = Position;
}

void UExtendedPhysicsComponent::GetPVA(FVector& Position, FVector& Velocity, FVector& Acceleration)
{
    FScopeLock(&this->cs);
    Position = position;
    Velocity = velocity;
    Acceleration = acceleration;
}

void UExtendedPhysicsComponent::SetPVA(FVector& Position, FVector& Velocity, FVector& Acceleration)
{
    FScopeLock(&this->cs);
    this->position = Position;
    this->velocity = Velocity;
    this->acceleration = Acceleration;
}

UStaticMeshComponent* UExtendedPhysicsComponent::GetStaticMesh()
{
    FScopeLock(&this->cs);
    return this->static_mesh;
}

void UExtendedPhysicsComponent::SetStaticMesh(UStaticMeshComponent* StaticMesh)
{
    FScopeLock(&this->cs);
    this->static_mesh = StaticMesh;
}

FVector z = FVector(0.0, 0.0, 0.0);

// Called every frame
void UExtendedPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    FScopeLock(&this->cs);
    if (static_mesh != NULL)
    {
        FVector dv = DeltaTime * acceleration;
        FVector dp = DeltaTime * (velocity + 0.5 * dv);
        position += dp;
        velocity += dv;

        // Since client-side collisions are approximations, set the velocities they
        // got to zero on every tick to undo any local collision events.
        static_mesh->SetPhysicsLinearVelocity(z, false, NAME_None);
        //static_mesh->SetAllPhysicsAngularVelocity(z, false);
        static_mesh->SetAllPhysicsPosition(position);

        // See: https://docs.unrealengine.com/latest/INT/API/Runtime/Engine/Components/UPrimitiveComponent/AddForce/index.html
        // Setting this or the velocity has a considerable impact on peformance, 
        // due to UE's limitations in physics calculation throughput. Setting either
        // of these properties takes the objects out of physics sleep, but setting the
        // position does not.
        // 
        // To address this, velocity and acceleration are calculated into a position and
        // velocity component, and the position is updated on each tick.
        //
        //static_mesh->AddForce(acceleration, NAME_None, true);
    }
}
