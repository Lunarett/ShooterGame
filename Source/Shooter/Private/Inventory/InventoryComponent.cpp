#include "Inventory/InventoryComponent.h"
#include "Weapon/WeaponBase.h"
#include "GameFramework/Actor.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::AddWeapon(AWeaponBase* InWeapon)
{
	if (InWeapon && !WeaponList.Contains(InWeapon))
	{
		WeaponList.Add(InWeapon);
		if (!EquippedWeapon)
		{
			EquippedWeapon = InWeapon;
		}
	}
}

void UInventoryComponent::RemoveWeapon(AWeaponBase* InWeapon)
{
	if (InWeapon && WeaponList.Contains(InWeapon))
	{
		if (EquippedWeapon == InWeapon)
		{
			EquipNextWeapon();
		}
		WeaponList.Remove(InWeapon);
	}
}

void UInventoryComponent::AddAmmo(FGameplayTag AmmoTypeTag, int32 AmmoAmount)
{
	if (AmmoAmount <= 0) return;

	int32& CurrentAmount = AmmoMap.FindOrAdd(AmmoTypeTag);
	CurrentAmount += AmmoAmount;
}

void UInventoryComponent::ConsumeAmmo(FGameplayTag AmmoTypeTag, int32 AmmoAmount)
{
	if (AmmoAmount <= 0) return;

	int32& CurrentAmount = AmmoMap.FindOrAdd(AmmoTypeTag);
	CurrentAmount = FMath::Max(CurrentAmount - AmmoAmount, 0);
}

void UInventoryComponent::EquipNextWeapon()
{
	if (WeaponList.Num() <= 1) return;

	TArray<AWeaponBase*> WeaponArray = WeaponList.Array();
	const int32 CurrentIndex = WeaponArray.IndexOfByKey(EquippedWeapon);
	const int32 NewIndex = (CurrentIndex + 1) % WeaponArray.Num();

	EquipWeaponByIndex(NewIndex);
}

void UInventoryComponent::EquipPreviousWeapon()
{
	if (WeaponList.Num() <= 1) return;

	TArray<AWeaponBase*> WeaponArray = WeaponList.Array();
	const int32 CurrentIndex = WeaponArray.IndexOfByKey(EquippedWeapon);
	const int32 NewIndex = (CurrentIndex - 1 + WeaponArray.Num()) % WeaponArray.Num();

	EquipWeaponByIndex(NewIndex);
}

void UInventoryComponent::EquipWeaponByIndex(const int32 InIndex)
{
	TArray<AWeaponBase*> WeaponArray = WeaponList.Array();
	if (WeaponArray.IsValidIndex(InIndex))
	{
		EquippedWeapon = WeaponArray[InIndex];
	}
}