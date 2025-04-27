// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PickupReceiver.generated.h"

struct FPickupData;

// This class does not need to be modified.
UINTERFACE()
class UPickupReceiver : public UInterface
{
	GENERATED_BODY()
};

/**
 * Add this interface to an ActorComponent or an Actor if you wish to interact
 * with the PickupActor and want to receive the pickup data
 */
class SHOOTER_API IPickupReceiver
{
	GENERATED_BODY()
	
public:
	/*
	 * When PickupActor gets overlapped this method will be called, and it will only pass the PickupData.
	 * Implement your desired behavior, and obtain the pickup data from the passed parameter
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pickup")
	void HandlePickup(const FPickupData& PickupData);
};