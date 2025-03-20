// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "AmmoProvider.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UAmmoProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * This interface is for the weapon requesting for ammo from its owner
 */
class SHOOTER_API IAmmoProvider
{
	GENERATED_BODY()

public:
	/**
	 * Requests ammo of the specified type from the provider.
	 * @param AmmoType The type of ammo requested, represented as a Gameplay Tag.
	 * @param RequestedAmount The amount of ammo requested.
	 * @return The amount of ammo successfully provided.
	 */
	virtual int32 RequestAmmo(FGameplayTag AmmoType, int32 RequestedAmount) = 0;
};