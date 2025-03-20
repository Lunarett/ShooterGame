#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "WeaponLineTrace.generated.h"

UCLASS(Abstract, Blueprintable)
class SHOOTER_API AWeaponLineTrace : public AWeaponBase
{
	GENERATED_BODY()

public:
	AWeaponLineTrace();

protected:
	virtual void FireWeapon() override;
};
