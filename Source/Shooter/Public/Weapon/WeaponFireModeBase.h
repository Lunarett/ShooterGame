#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WeaponFireModeBase.generated.h"

/**
 * Provides a base for weapon fire modes, defining how firing behaviors are executed and stopped.
 */
UCLASS(Abstract, Blueprintable)
class SHOOTER_API UWeaponFireModeBase : public UObject
{
	GENERATED_BODY()

protected:
	float LastFireTime = 0.0f;
	
public:
	/** Initializes the firing behavior, setting up execution logic with a specified fire rate. */
	virtual void InitializeFire(class UWorld* World, FTimerHandle& TimerHandle, FTimerDelegate FireDelegate, float FireRate) PURE_VIRTUAL(UWeaponFireModeBase::InitializeFire, );

	/** Stops any active firing behavior. */
	virtual void StopFire(UWorld* World, FTimerHandle& TimerHandle) PURE_VIRTUAL(UWeaponFireModeBase::StopFire, );

protected:
	bool CanFire(const UWorld* World, const float FireRate);
};

/**
 * Implements single-shot firing behavior.
 */
UCLASS()
class SHOOTER_API UWeaponFireModeSingle : public UWeaponFireModeBase
{
	GENERATED_BODY()

public:
	/** Executes a single-shot fire, respecting the fire rate. */
	virtual void InitializeFire(UWorld* World, FTimerHandle& TimerHandle, FTimerDelegate FireDelegate, float FireRate) override;

	/** Single-shot mode has no stopping logic. */
	virtual void StopFire(UWorld* World, FTimerHandle& TimerHandle) override;
};

/**
 * Implements automatic firing behavior.
 */
UCLASS()
class SHOOTER_API UWeaponFireModeAutomatic : public UWeaponFireModeBase
{
	GENERATED_BODY()

public:
	/** Continuously executes fire at regular intervals defined by the fire rate. */
	virtual void InitializeFire(UWorld* World, FTimerHandle& TimerHandle, FTimerDelegate FireDelegate, float FireRate) override;

	/** Stops automatic firing. */
	virtual void StopFire(UWorld* World, FTimerHandle& TimerHandle) override;
};
