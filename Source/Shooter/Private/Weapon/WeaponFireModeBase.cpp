#include "Weapon/WeaponFireModeBase.h"
#include "TimerManager.h"

bool UWeaponFireModeBase::CanFire(const UWorld* World, const float FireRate)
{
	if (!World)
	{
		return false;
	}

	if (const float CurrentTime = World->GetTimeSeconds(); LastFireTime + FireRate <= CurrentTime)
	{
		// Update LastFireTime and allow firing
		LastFireTime = CurrentTime;
		return true;
	}

	return false;
}

// Single-shot fire mode implementation
void UWeaponFireModeSingle::InitializeFire(UWorld* World, FTimerHandle& TimerHandle, FTimerDelegate FireDelegate,
                                           float FireRate)
{
	if (FireDelegate.IsBound() && CanFire(World, FireRate))
	{
		FireDelegate.Execute();
	}
}

void UWeaponFireModeSingle::StopFire(UWorld* World, FTimerHandle& TimerHandle)
{
}

// Automatic fire mode implementation
void UWeaponFireModeAutomatic::InitializeFire(UWorld* World, FTimerHandle& TimerHandle, FTimerDelegate FireDelegate,
                                              float FireRate)
{
	if (FireDelegate.IsBound() && CanFire(World, FireRate))
	{
		// Execute fire immediately
		FireDelegate.Execute();

		// Set a timer to continue firing automatically
		World->GetTimerManager().SetTimer(
			TimerHandle,
			FireDelegate,
			FireRate,
			true
		);
	}
}

void UWeaponFireModeAutomatic::StopFire(UWorld* World, FTimerHandle& TimerHandle)
{
	if (World)
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
}
