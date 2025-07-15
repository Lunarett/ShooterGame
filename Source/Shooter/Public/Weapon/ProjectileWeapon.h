#pragma once

#include "CoreMinimal.h"
#include "Weapon/WeaponBase.h"
#include "ProjectileWeapon.generated.h"

UCLASS()
class SHOOTER_API AProjectileWeapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	AProjectileWeapon();

protected:
        virtual void FireWeapon() override;

       UFUNCTION(Client, Reliable)
       void ClientSpawnLocalProjectile(const FVector& SpawnLocation, const FRotator& SpawnRotation, const FVector& Direction);

	// The projectile class to spawn.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class AProjectileBase> ProjectileClass;

	// Bullet spread in degrees.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Projectile")
	float BulletSpread;
};
