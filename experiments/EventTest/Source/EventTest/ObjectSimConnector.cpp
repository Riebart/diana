// Fill out your copyright notice in the Description page of Project Settings.

#include "EventTest.h"
#include "ObjectSimConnector.h"

#include "Array.h"

// Sets default values
AObjectSimConnector::AObjectSimConnector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AObjectSimConnector::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AObjectSimConnector::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

TArray<struct FDirectoryItem> AObjectSimConnector::DirectoryListing(FString Type, TArray<struct FDirectoryItem> Items)
{
    TArray<struct FDirectoryItem> ret;
    struct FDirectoryItem i;
    i.ItemName = FString("2560 x 1440 pixels^2");
    i.ItemID = 1;
    ret.Add(i);
    i.ItemName = FString("1920 x 1080 pixels^2");
    i.ItemID = 1;
    ret.Add(i);
    i.ItemName = FString("1280 x 720 pixels^2");
    i.ItemID = 1;
    ret.Add(i);
    return ret;
}

void AObjectSimConnector::CreateShip(int32 ServerID)
{
}

void AObjectSimConnector::JoinShip(int32 ServerID)
{
}

void AObjectSimConnector::RenameShip(int32 ServerID, FString NewShipName)
{
}

void AObjectSimConnector::Ready(int32 ServerID)
{
}
