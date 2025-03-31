#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class SHOOTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

protected:
	// Components
	UPROPERTY(VisibleDefaultsOnly, Category = "Projectile")
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere, Category = "Effects")
	UNiagaraComponent* ProjectileEffectComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* ImpactEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UMaterialInterface* ImpactDecalMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	FVector DecalSize;
	

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float LifeTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage")
	float DamageAmount;

private:
	FTimerHandle LifetimeTimerHandle;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
public:
	// Call this when firing projectile. This determines the direction the projectile gets fired in
	void FireInDirection(const FVector& ShootDirection);

private:
	// VFX
	void SpawnImpactEffect(const FVector& Location, const FRotator& Rotation);
	void SpawnDecal(const FVector& Location, const FRotator& Rotation);

	void OnLifetimeExpired();
};