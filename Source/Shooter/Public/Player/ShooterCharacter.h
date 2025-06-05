// ShooterCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Weapon/AmmoProvider.h"
#include "ShooterCharacter.generated.h"

UENUM(BlueprintType)
enum class EPlayerViewMode : uint8
{
	FirstPerson		UMETA(DisplayName = "First Person"),
	ThirdPerson		UMETA(DisplayName = "Third Person")
};

class USkeletalMeshComponent;
class USpringArmComponent;
class USceneComponent;
class UCameraComponent;
class AWeaponBase;
class UInventoryComponent;
class UAnimMontage;

UCLASS()
class SHOOTER_API AShooterCharacter : public ACharacter, public IAmmoProvider
{
	GENERATED_BODY()

public:
	AShooterCharacter();

protected:
	UPROPERTY(BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USceneComponent* FPRootSceneComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USpringArmComponent* FPMeshRootSpringArmComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USceneComponent* OffsetRootSceneComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USkeletalMeshComponent* FPMesh;

	UPROPERTY(BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USpringArmComponent* FPCameraRootSpringArmComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USkeletalMeshComponent* CameraSkeletalMesh;

	UPROPERTY(BlueprintReadWrite, Category = "Shooter Character|FP Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USpringArmComponent* TPSpringArmComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooter Character|FP Components")
	UInventoryComponent* InventoryComponent;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Weapon")
	FName WeaponSocketName;
	
	UPROPERTY(ReplicatedUsing=OnRep_WeaponActor, BlueprintReadOnly, Category = "Shooter Character|Weapon")
	AWeaponBase* WeaponActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Animations")
	UAnimMontage* EquipMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character")
	EPlayerViewMode ViewMode;
	

private:
	UPROPERTY(Replicated)
	float AimOffsetYaw;

	UPROPERTY(Replicated)
	float AimOffsetPitch;

	bool bCanInteractWithWeapon;

	FTimerHandle EquipTimerHandle;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UFUNCTION()
	void OnWeaponEquipped(float EquipCooldownDuration);
	
public:
	UFUNCTION(BlueprintCallable)
	void BeginFire();

	UFUNCTION(BlueprintCallable)
	void EndFire();

	UFUNCTION(BlueprintCallable)
	void ReloadWeapon();

	virtual int32 RequestAmmo_Implementation(FGameplayTag AmmoType, int32 RequestedAmount) override;

	void SetPlayerViewMode(EPlayerViewMode NewViewMode);

private:
	void AttachWeaponToMesh();
	void PlayEquipMontage(bool bReverse);

	UFUNCTION()
	void OnRep_WeaponActor();

public:
	UFUNCTION(BlueprintCallable, Category = "AimOffset")
	FORCEINLINE void GetAimOffsetValues(float& OutYaw, float& OutPitch) const
	{
		OutYaw = AimOffsetYaw;
		OutPitch = AimOffsetPitch;
	}

	FORCEINLINE bool IsPawnAIControlled() const { return Controller && Controller->IsA<AAIController>(); }
	FORCEINLINE USkeletalMeshComponent* GetFPMeshComponent() const { return FPMesh; }
	FORCEINLINE USkeletalMeshComponent* GetTPMeshComponent() const { return GetMesh(); }
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	FORCEINLINE EPlayerViewMode GetViewMode() const { return ViewMode; }
};