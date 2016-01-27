// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ObjectSimConnector.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FDirectoryItem
{
    GENERATED_USTRUCT_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = HUD)
        FString ItemName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = HUD)
        int32 ItemID;
};

UCLASS()
class EVENTTEST_API AObjectSimConnector : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AObjectSimConnector();

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Called every frame
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server information", meta = (DisplayName = "Server IPv4 Address"))
        FString host = "127.0.0.1";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server information", meta = (DisplayName = "Server TCP Port"))
        int32 port = 5506;

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        TArray<struct FDirectoryItem> DirectoryListing(int32 ClientID, FString Type, TArray<struct FDirectoryItem> Items);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void CreateShip(int32 ClientID, int32 ServerID);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void JoinShip(int32 ClientID, int32 ServerID);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void RenameShip(int32 ClientID, int32 ServerID, FString NewShipName);

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
        void Ready(int32 ClientID, int32 ServerID);
};
