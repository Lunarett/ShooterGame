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
class UNiagaraSystem;
class USceneComponent;
class UWeaponRecoilComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);

USTRUCT(BlueprintType)
struct FWeaponAmmoData
{
	GENERATED_BODY()

	/* GameplayTag that will hold all types of ammo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ammo")
	FGameplayTag AmmoTypeTag;

	/* Total size of ammo per Magazine */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ammo")
	int32 ClipSize;

	/* Setting this to true will prevent from requesting ammo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ammo Cheats")
	bool bInfiniteAmmo;

	/* Setting this to true will prevent the weapon from reloading */
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

	/* Animation that will be played from First-Person Perspective */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Anim")
	UAnimMontage* FPAnimationMontage;

	/* Animation that will be played from Third-Person Perspective */
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	bool bAutoReload;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float ReloadDuration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	FWeaponAmmoData AmmoData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	FWeaponAnimationData WeaponFireAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	FWeaponAnimationData WeaponReloadAnimation;
	
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Components", meta = (AllowPrivateAccess = true))
	UWeaponRecoilComponent* RecoilComponent;
	
private:
	UPROPERTY(Transient)
	UWeaponFireModeBase* FireModeBehavior;

	UPROPERTY(Replicated)
	AShooterCharacter* OwnerShooterCharacter;

	UPROPERTY(Replicated)
	int32 CurrentClipAmmo;

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;

	bool bIsReloadingWeapon;
	
protected:
	UPROPERTY(BlueprintAssignable)
	FOnWeaponFired OnWeaponFiredDelegate;
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	

// Weapon Fire
public:
	/* Based on FireMode, Begin the Fire Behavior */
	virtual void BeginFire();

	/* Terminate Fire Behavior */
	virtual void EndFire();

protected:
	/* RPC - Executes BeginFire on the server */
	UFUNCTION(reliable, server, WithValidation)
	void ServerBeginFire();

	/* RPC - Executes EndFire on the server */
	UFUNCTION(reliable, server, WithValidation)
	void ServerEndFire();
	
protected:
	/* Triggers everything that should usually happen when weapon is fired */
	virtual void HandleWeaponFire();

	/* Abstract Method - Actual Fire Logic - Override it in your child class and implement the Fire Logic */
	virtual void FireWeapon() PURE_VIRTUAL(AShooterWeapon::FireWeapon,);


	
// Weapon Reload
public:
	/* If Ammo is < MaxClipSize Reload Weapon */
	virtual void BeginReload();
	
private:
	/* Called when reload is completed */
	virtual void EndReload();

	/* RPC - Executes BeginReload on the server */
	UFUNCTION(reliable, server, WithValidation)
	void ServerBeginReload();


	
// VFX
private:
	/* Spawns Muzzle Niagara Particle Effect */
	void PlayMuzzleEffect();

	// Multicast - Calls PlayMuzzleEffect
	UFUNCTION(NetMulticast, reliable)
	void MulticastPlayMuzzleEffect();


	
// Weapon Animation
private:
	/* Plays Fire Animation */
	void PlayAnimationMontage(FWeaponAnimationData AnimationData);
	
	// Animations - Multicast
	UFUNCTION(NetMulticast, reliable)
	void MulticastPlayWeaponFireAnimation();

	UFUNCTION(NetMulticast, reliable)
	void MulticastPlayWeaponReloadAnimation();



// Helpers & Getters
protected:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CanReload() const;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FORCEINLINE AShooterCharacter* GetOwnerShooterCharacter() const { return OwnerShooterCharacter; }
	
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FORCEINLINE USkeletalMeshComponent* GetFPWeaponMeshComponent() const { return FPWeaponMeshComponent; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FORCEINLINE USkeletalMeshComponent* GetTPWeaponMeshComponent() const { return TPWeaponMeshComponent; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FORCEINLINE FVector GetWeaponOffset() const { return WeaponOffset; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FORCEINLINE FWeaponAmmoData GetAmmoData() const { return AmmoData; }
};