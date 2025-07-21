// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Debug/LoggerMacros.h"
#include "Pickup/PickupActor.h"
#include "Pickup/PickupReceiver.h"
#include "Net/UnrealNetwork.h"
#include "InventoryComponent.generated.h"

class AWeaponBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquipped, float, CoolDownDuration);

USTRUCT()
struct FAmmoEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag AmmoTypeTag;

    UPROPERTY()
    int32 Amount = 0;
};

/*
 * Very simple inventory system that holds ammo and scrolls between weapons
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTER_API UInventoryComponent : public UActorComponent, public IPickupReceiver
{
        GENERATED_BODY()

public:
        UInventoryComponent();

       virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon")
	float EquipCooldownDuration = 0.7f;
	
	/* Stores amount of ammo the Pawn/Character possesses */
       UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Ammo")
       TMap<FGameplayTag, int32> AmmoMap;

       UPROPERTY(ReplicatedUsing=OnRep_AmmoEntries)
       TArray<FAmmoEntry> AmmoEntries;

	/* List of all weapons the player will begin with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Weapon")
	TSet<TSubclassOf<AWeaponBase>> StartingWeaponList;

	/* The *Actual* weapons list inside the inventory */
       UPROPERTY(BlueprintReadWrite, Replicated, Category = "Inventory|Weapon")
       TArray<AWeaponBase*> WeaponList;

	/* Currently equipped weapon. nullptr on this means that the weapon is currently unequipped */
       UPROPERTY(BlueprintReadWrite, ReplicatedUsing=OnRep_EquippedWeapon, Category = "Inventory|Weapon")
       AWeaponBase* EquippedWeapon;

private:
	bool bIsEquipActive;
	
	/* Index of weapon pending to be equipped */
	int32 PendingEquipIndex = INDEX_NONE;

	/* Internal timer handle for switching */
	FTimerHandle EquipTimerHandle;

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnWeaponEquipped OnWeaponEquippedDelegate;
	
protected:
	virtual void BeginPlay() override;

public:
	/* Adds a new Weapon to inventory */
       UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
       void AddWeapon(TSubclassOf<AWeaponBase> InWeaponClass);

       UFUNCTION(Server, Reliable, WithValidation)
       void ServerAddWeapon(TSubclassOf<AWeaponBase> InWeaponClass);

	/* If Weapon exists in the list, remove it */
       UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
       void RemoveWeapon(AWeaponBase* InWeapon);


	/* Adds Ammo to AmmoMap */
       UFUNCTION(BlueprintCallable, Category = "Inventory|Ammo")
       void AddAmmo(FGameplayTag AmmoTypeTag, int32 AmmoAmount);

       UFUNCTION(Server, Reliable, WithValidation)
       void ServerAddAmmo(FGameplayTag AmmoTypeTag, int32 AmmoAmount);

	/* Deducts Ammo amount from AmmoMap */
       UFUNCTION(BlueprintCallable, Category = "Inventory|Ammo")
       void ConsumeAmmo(FGameplayTag AmmoTypeTag, int32 AmmoAmount);


	/* Equip the next Weapon from the list (Increment Index) (Circular Scrolling) */
       UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
       void EquipNextWeapon();

	/* Equip the previous Weapon from the list (Decrement Index) (Circular Scrolling) */
       UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
       void EquipPreviousWeapon();

	/* Internal helper method for selecting/equipping weapons */
       void EquipWeaponByIndex(const int32 InIndex);

       UFUNCTION(Server, Reliable, WithValidation)
       void ServerEquipWeaponByIndex(int32 InIndex);

private:
       /* Pickup Receiver Interface - Handle picking up Ammo or Weapon */
       virtual void HandlePickup_Implementation(const FPickupData& PickupData) override;

       UFUNCTION()
       void OnRep_EquippedWeapon();

       UFUNCTION()
       void OnRep_AmmoEntries();

       void SyncAmmoEntries();

	
	/* Handles hiding the other weapons in your inventory */
	void SetWeaponRenderFocus(const AWeaponBase* NewWeaponFocus);

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

	FORCEINLINE bool GetIsEquipActive() const { return bIsEquipActive; }
};