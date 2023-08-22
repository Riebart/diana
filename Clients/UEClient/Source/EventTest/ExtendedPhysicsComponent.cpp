// Fill out your copyright notice in the Description page of Project Settings.

#include "ExtendedPhysicsComponent.h"
#include "EventTest.h"


// Sets default values for this component's properties
UExtendedPhysicsComponent::UExtendedPhysicsComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
//    bWantsBeginPlay = true;
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
    set_accel = true;
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
    set_velocity = true;
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
    set_position = true;
    this->position = Position;
}

void UExtendedPhysicsComponent::GetPVA(FVector& Position, FVector& Velocity, FVector& Acceleration)
{
    FScopeLock(&this->cs);
    Position = position;
    Velocity = velocity;
    Acceleration = acceleration;
}

void UExtendedPhysicsComponent::SetPVA(FVector Position, FVector Velocity, FVector Acceleration)
{
    FScopeLock(&this->cs);
    set_position = true;
    this->position = Position;
    set_velocity = true;
    this->velocity = Velocity;
    set_accel = true;
    this->acceleration = Acceleration;
}

void UExtendedPhysicsComponent::motion_interpolation(double* t, FVector* p, int32 order, FVector* components)
{
    switch (order)
    {
    case 1:
    {
        components[0] = (p[1] - p[0]) / (t[1] - t[0]);
    }
    case 2:
    {
        FVector dp[] = { p[1] - p[0], p[2] - p[0] };
        double dt[] = { t[1] - t[0], t[2] - t[0] };

        components[0] = dp[1] * (dt[0] / (dt[1] * (dt[0] - dt[1]))) -
                        dp[0] * (dt[1] / (dt[0] * (dt[0] - dt[1])));

        components[1] = dp[0] * (2.0 / (dt[0] * (dt[0] - dt[1]))) -
                        dp[1] * (2.0 / (dt[1] * (dt[0] - dt[1])));

        //v = 0.5 * (dp[1] * (dt[0] / (dt[1] * (dt[0] - dt[1]))) -
        //    dp[0] * (dt[1] / (dt[0] * (dt[0] - dt[1])))) + (p[2] - p[1]) / (t[2] - t[1]);

        break;
    }
    default:
        break;
    }
}

float UExtendedPhysicsComponent::GetLocalSimThreshold()
{
    return this->local_physics_thresh;
}

void UExtendedPhysicsComponent::SetLocalSimThreshold(float Threshold)
{
    this->local_physics_thresh = Threshold;
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
        // Setting the acceleration or the velocity has a considerable impact on peformance, 
        // due to UE's limitations in physics calculation throughput. Setting either
        // of these properties takes the objects out of physics sleep, but setting the
        // position does not.
        // 
        // To address this, when the frametime is long enough, optimizations are made and 
        // velocity and acceleration are calculated into a position and velocity component, 
        // and the position is updated on each tick.

        // Only enable local physics if the frametime is low enough, OR if the frametime is
        // suficiently fast with physics off
        //if ((local_physics_enabled && (DeltaTime < local_physics_thresh)) || 
        //    (!local_physics_enabled && (DeltaTime < 0.1 * local_physics_thresh)))
        //{
            //if (!local_physics_enabled)
            //{
            //    UE_LOG(LogTemp, Warning, TEXT("ExtendedPhysics::LocalPhysics::ENABLING %f"), DeltaTime / local_physics_thresh);
            //    local_physics_enabled = true;
            //}

            //if (set_position)
            //{
            //    static_mesh->SetAllPhysicsPosition(position);
            //    set_position = false;
            //}

            //if (set_velocity)
            //{
            //    static_mesh->SetPhysicsLinearVelocity(velocity, false, NAME_None);
            //    set_velocity = false;
            //}

            //// See: https://docs.unrealengine.com/latest/INT/API/Runtime/Engine/Components/UPrimitiveComponent/AddForce/index.html
            //static_mesh->AddForce(acceleration, NAME_None, true);
        //}
        //else
        //{
            //if (local_physics_enabled)
            //{
            //    UE_LOG(LogTemp, Warning, TEXT("ExtendedPhysics::LocalPhysics::DISABLING %f"), DeltaTime / local_physics_thresh);
            //    local_physics_enabled = false;
            //}

        FVector dv = DeltaTime * acceleration;
        FVector dp = DeltaTime * (velocity + 0.5 * dv);
        position += dp;
        velocity += dv;

        // Since client-side collisions are approximations, set the velocities they
        // got to zero on every tick to undo any local collision events.
        static_mesh->SetPhysicsLinearVelocity(z, false, NAME_None);
//        static_mesh->SetAllPhysicsAngularVelocity(z, false);
        static_mesh->SetAllPhysicsPosition(position);
        //}
    }
}
