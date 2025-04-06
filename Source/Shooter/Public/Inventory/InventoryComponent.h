// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Debug/LoggerMacros.h"
#include "Weapon/AmmoProvider.h"
#include "InventoryComponent.generated.h"

class AWeaponBase;

/*
 * Very simple inventory system that holds ammo and scrolls between weapons
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTER_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

protected:
	/* Stores amount of ammo the Pawn/Character possesses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Ammo")
	TMap<FGameplayTag, int32> AmmoMap;

	/* Stores different unique weapons the Pawn/Character possesses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon")
	TSet<AWeaponBase*> WeaponList;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory|Weapon")
	AWeaponBase* EquippedWeapon;

public:
	/* Adds a new Weapon to inventory */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	void AddWeapon(AWeaponBase* InWeapon);

	/* If Weapon exists in the list, remove it */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	void RemoveWeapon(AWeaponBase* InWeapon);


	/* Adds Ammo to AmmoMap */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Ammo")
	void AddAmmo(FGameplayTag AmmoTypeTag, int32 AmmoAmount);

	/* Deducts Ammo amount from AmmoMap */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Ammo")
	void ConsumeAmmo(FGameplayTag AmmoTypeTag, int32 AmmoAmount);


	/* Equip the next Weapon from the list (Increment Index) (Circular Scrolling) */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	void EquipNextWeapon();

	/* Equip the previous Weapon from the list (Decrement Index) (Circular Scrolling) */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	void EquipPreviousWeapon();

private:
	/* Internal helper method for selecting/equipping weapons */
	void EquipWeaponByIndex(const int32 InIndex);

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	FORCEINLINE int32 GetAmmo(const FGameplayTag AmmoTypeTag) const
	{
		if (!AmmoTypeTag.IsValid())
		{
			LOG_ERROR("Ammo Type is invalid");
			return 0;
		}
		
		return AmmoMap[AmmoTypeTag];
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	FORCEINLINE AWeaponBase* GetEquippedWeapon() const
	{
		return EquippedWeapon;
	}
};