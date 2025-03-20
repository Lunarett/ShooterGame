#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class AShooterCharacter;
class UWeaponFireModeBase;
class USkeletalMeshComponent;
class UAnimMontage;
class USoundCue;
class UForceFeedbackEffect;
class UNiagaraSystem;
class USceneComponent;
class UWeaponRecoilComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);

USTRUCT(BlueprintType)
struct FWeaponAmmoData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ammo")
	FGameplayTag AmmoTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ammo")
	int32 ClipSize;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ammo Cheats")
	bool bInfiniteAmmo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ammo Cheats")
	bool bInfiniteClip;

	FWeaponAmmoData()
		: AmmoTypeTag(),
		  ClipSize(30),
		  bInfiniteAmmo(false),
		  bInfiniteClip(false)
	{
	}
};

USTRUCT(BlueprintType)
struct FWeaponAnimationData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Anim")
	UAnimMontage* FPAnimationMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Anim")
	UAnimMontage* TPAnimationMontage;

	FWeaponAnimationData()
		: FPAnimationMontage(nullptr)
		, TPAnimationMontage(nullptr)
	{
	}
};

UCLASS(Abstract, Blueprintable)
class SHOOTER_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UWeaponFireModeBase> FireModeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FVector WeaponOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float FireRate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	FWeaponAnimationData WeaponFireAnimation;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	FName WeaponMuzzleSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	UNiagaraSystem* MuzzleFlashEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TSubclassOf<UCameraShakeBase> FireCameraShake;
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Components", meta = (AllowPrivateAccess = true))
	USceneComponent* WeaponSceneRootComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Components", meta = (AllowPrivateAccess = true))
	USkeletalMeshComponent* FPWeaponMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Components", meta = (AllowPrivateAccess = true))
	USkeletalMeshComponent* TPWeaponMeshComponent;
	
private:
	UPROPERTY(Transient)
	UWeaponFireModeBase* FireModeBehavior;

	UPROPERTY(Replicated)
	AShooterCharacter* OwnerShooterCharacter;

	FTimerHandle FireTimerHandle;

	UPROPERTY(BlueprintAssignable)
	FOnWeaponFired OnWeaponFiredDelegate;
	
protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	virtual void BeginFire();
	virtual void EndFire();

protected:
	virtual void HandleWeaponFire();
	virtual void FireWeapon() PURE_VIRTUAL(AShooterWeapon::FireWeapon,);

	UFUNCTION(reliable, server, WithValidation)
	void ServerBeginFire();

	UFUNCTION(reliable, server, WithValidation)
	void ServerEndFire();

protected:
	// Animations - Multicast
	UFUNCTION(NetMulticast, reliable)
	void MulticastPlayWeaponFireAnimation();

	// Effects - Multicast
	UFUNCTION(NetMulticast, reliable)
	void MulticastPlayMuzzleEffect();

private:
	// Animations
	void PlayWeaponFireAnimation();

	// Effects
	void PlayMuzzleEffect();

protected:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CanReload() const;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FORCEINLINE AShooterCharacter* GetOwnerShooterCharacter() const { return OwnerShooterCharacter; }
	
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USkeletalMeshComponent* GetFPWeaponMeshComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USkeletalMeshComponent* GetTPWeaponMeshComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FORCEINLINE FVector GetWeaponOffset() const { return WeaponOffset; } 
};