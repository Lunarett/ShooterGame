#include "Inventory/InventoryComponent.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponBase.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent()
{
        PrimaryComponentTick.bCanEverTick = false;
        SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);

        DOREPLIFETIME(UInventoryComponent, AmmoMap);
        DOREPLIFETIME(UInventoryComponent, WeaponList);
        DOREPLIFETIME(UInventoryComponent, EquippedWeapon);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld() && GetOwner() && !StartingWeaponList.IsEmpty())
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.Instigator = Cast<APawn>(GetOwner());
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// Reserve WeaponList
		WeaponList.Reserve(StartingWeaponList.Num());

		// Iterate through each weapon list and spawn the weapon
		for (TSubclassOf<AWeaponBase> WeaponClass : StartingWeaponList)
		{
			if (!WeaponClass)
			{
				LOG_WARN_SCREEN("Invalid weapon class, wont spawn that weapon")
				continue;
			}

			AWeaponBase* SpawnedWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponClass, SpawnParameters);
			if (!SpawnedWeapon)
			{
				LOG_ERROR_SCREEN("Failed to spawn a weapon")
				continue;
			}

			// Add spawned weapon to list
			WeaponList.Add(SpawnedWeapon);
		}
	}
}

void UInventoryComponent::AddWeapon(TSubclassOf<AWeaponBase> InWeaponClass)
{
        if (!HasAuthority())
        {
                ServerAddWeapon(InWeaponClass);
                return;
        }

        if (!InWeaponClass)
	{
		LOG_ERROR_SCREEN("Failed to add weapon. You passed an invalid weapon class");
		return;
	}

	// Check if we already own a weapon of this class
	for (const AWeaponBase* Weapon : WeaponList)
	{
		// If our weapon exists, just add its ammo to the inventory
		if (Weapon && Weapon->GetClass() == InWeaponClass)
		{
			const AWeaponBase* DefaultWeapon = InWeaponClass->GetDefaultObject<AWeaponBase>();
			if (DefaultWeapon)
			{
				AddAmmo(DefaultWeapon->GetAmmoData().AmmoTypeTag, DefaultWeapon->GetAmmoData().ClipSize);
			}
			
			return;
		}
	}

	// If our weapon we are adding doesn't yet exist, Spawn it & add it
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(InWeaponClass, SpawnParams);
	if (!NewWeapon)
	{
		LOG_ERROR_SCREEN("Failed to spawn new weapon");
		return;
	}

	// Add weapon to list
	WeaponList.Add(NewWeapon);

	// Equip newly added weapon
	EquipWeaponByIndex(WeaponList.Num() - 1);
}

void UInventoryComponent::RemoveWeapon(AWeaponBase* InWeapon)
{
	if (!InWeapon)
	{
		LOG_ERROR_SCREEN("Failed to remove weapon. You passed an invalid weapon");
		return;
	}

	if (!WeaponList.Contains(InWeapon))
	{
		LOG_WARN("Failed to remove weapon. That weapon no longer exists in the list");
		return;
	}

	if (EquippedWeapon == InWeapon)
	{
		EquipNextWeapon();
	}

	WeaponList.Remove(InWeapon);
}

void UInventoryComponent::AddAmmo(FGameplayTag AmmoTypeTag, int32 AmmoAmount)
{
        if (!HasAuthority())
        {
                ServerAddAmmo(AmmoTypeTag, AmmoAmount);
                return;
        }

        if (AmmoAmount <= 0)
        {
                return;
        }

	int32& CurrentAmount = AmmoMap.FindOrAdd(AmmoTypeTag);
	CurrentAmount += AmmoAmount;
}

void UInventoryComponent::ConsumeAmmo(FGameplayTag AmmoTypeTag, int32 AmmoAmount)
{
	if (AmmoAmount <= 0)
	{
		return;
	}

	int32& CurrentAmount = AmmoMap.FindOrAdd(AmmoTypeTag);
	CurrentAmount = FMath::Max(CurrentAmount - AmmoAmount, 0);
}

void UInventoryComponent::EquipNextWeapon()
{
	if (bIsEquipActive || !IsValid(EquippedWeapon) || WeaponList.Num() <= 1)
	{
		return;
	}

	const int32 CurrentIndex = WeaponList.IndexOfByKey(EquippedWeapon);
	const int32 NewIndex = (CurrentIndex + 1) % WeaponList.Num();

	EquipWeaponByIndex(NewIndex);
}

void UInventoryComponent::EquipPreviousWeapon()
{
	if (bIsEquipActive || !IsValid(EquippedWeapon) || WeaponList.Num() <= 1)
	{
		return;
	}

	const int32 CurrentIndex = WeaponList.IndexOfByKey(EquippedWeapon);
	const int32 NewIndex = (CurrentIndex - 1 + WeaponList.Num()) % WeaponList.Num();

	EquipWeaponByIndex(NewIndex);
}

void UInventoryComponent::EquipWeaponByIndex(const int32 InIndex)
{
        if (!WeaponList.IsValidIndex(InIndex))
        {
                return;
        }

        if (!HasAuthority())
        {
                ServerEquipWeaponByIndex(InIndex);
                return;
        }

	bIsEquipActive = true;

	AWeaponBase* NewWeapon = WeaponList[InIndex];
	EquippedWeapon = NewWeapon;

	// Hide the other weapons and show the active one
	SetWeaponRenderFocus(EquippedWeapon);

	// Broadcast delegate for the owner to handle equip logic
	OnWeaponEquippedDelegate.Broadcast(EquipCooldownDuration);

	// Start timer for equip cool down
	GetWorld()->GetTimerManager().SetTimer(EquipTimerHandle, [this]()
	{
		bIsEquipActive = false;
	}, EquipCooldownDuration, false);
}

void UInventoryComponent::HandlePickup_Implementation(const FPickupData& PickupData)
{
	switch (PickupData.PickupType)
	{
	case EPickupType::Weapon:
		AddWeapon(PickupData.WeaponClass);
		break;
	case EPickupType::Ammo:
		AddAmmo(PickupData.AmmoTypeTag, PickupData.AmmoAmount);
		break;
	}
}

void UInventoryComponent::SetWeaponRenderFocus(const AWeaponBase* NewWeaponFocus)
{
	if (!IsValid(NewWeaponFocus))
	{
		return;
	}

	// Toggle off all the weapons besides the active/equipped weapon
	for (AWeaponBase* Weapon : WeaponList)
	{
		const bool bIsFocused = (Weapon == NewWeaponFocus);
		Weapon->SetActorHiddenInGame(!bIsFocused);
		Weapon->SetActorEnableCollision(bIsFocused);
		Weapon->SetActorTickEnabled(bIsFocused);
		Weapon->EndFire();
        }
}

void UInventoryComponent::OnRep_EquippedWeapon()
{
        SetWeaponRenderFocus(EquippedWeapon);
        OnWeaponEquippedDelegate.Broadcast(EquipCooldownDuration);
}

bool UInventoryComponent::ServerAddWeapon_Validate(TSubclassOf<AWeaponBase> InWeaponClass)
{
        return true;
}

void UInventoryComponent::ServerAddWeapon_Implementation(TSubclassOf<AWeaponBase> InWeaponClass)
{
        AddWeapon(InWeaponClass);
}

bool UInventoryComponent::ServerAddAmmo_Validate(FGameplayTag AmmoTypeTag, int32 AmmoAmount)
{
        return true;
}

void UInventoryComponent::ServerAddAmmo_Implementation(FGameplayTag AmmoTypeTag, int32 AmmoAmount)
{
        AddAmmo(AmmoTypeTag, AmmoAmount);
}

bool UInventoryComponent::ServerEquipWeaponByIndex_Validate(int32 InIndex)
{
        return true;
}

void UInventoryComponent::ServerEquipWeaponByIndex_Implementation(int32 InIndex)
{
        EquipWeaponByIndex(InIndex);
}
