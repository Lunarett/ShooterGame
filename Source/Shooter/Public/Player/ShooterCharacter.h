// ShooterCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLookInputChanged, FVector2D, MouseDelta);

class USkeletalMeshComponent;
class USpringArmComponent;
class USceneComponent;
class UCameraComponent;
class UInputAction;
class AWeaponBase;
struct FInputActionValue;

UCLASS()
class SHOOTER_API AShooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AShooterCharacter();

	UPROPERTY(BlueprintAssignable)
	FOnLookInputChanged OnLookInputChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USceneComponent* FPRootSceneComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USpringArmComponent* FPMeshRootSpringArmComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USceneComponent* OffsetRootSceneComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USkeletalMeshComponent* FPMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USpringArmComponent* CameraRootSpringArmComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooter Character|FP Components")
	USkeletalMeshComponent* CameraSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooter Character|FP Components")
	UCameraComponent* CameraComponent;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character")
	float LookSensitivity = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Weapon")
	FName WeaponSocketName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Weapon")
	TSubclassOf<AWeaponBase> WeaponClass;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponActor, BlueprintReadOnly, Category = "Shooter Character|Weapon")
	AWeaponBase* WeaponActor;

private:
	/** Input Actions */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Input",
		meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Input",
		meta = (AllowPrivateAccess = "true"))
	UInputAction* LookInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Input",
		meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Input",
		meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Input",
	meta = (AllowPrivateAccess = "true"))
	UInputAction* FireInputAction;

private:
	UPROPERTY(Replicated)
	float AimOffsetYaw;

	UPROPERTY(Replicated)
	float AimOffsetPitch;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character")
	float BaseMovementSpeed;

public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void HandleMoveInput(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void HandleBeginFireInput(const FInputActionValue& Value);
	void HandleEndFireInput(const FInputActionValue& Value);

public:
	UFUNCTION(BlueprintCallable)
	void BeginFire();

	UFUNCTION(BlueprintCallable)
	void EndFire();

protected:
	void SpawnWeapon();
	void AttachWeaponToMesh();

	UFUNCTION()
	void OnRep_WeaponActor();

public:
	UFUNCTION(BlueprintCallable, Category = "Shooter Character")
	bool IsPawnAIControlled() const;
	
	UFUNCTION(BlueprintCallable, Category = "AimOffset")
	void GetAimOffsetValues(float& OutYaw, float& OutPitch) const;
	
	FORCEINLINE USkeletalMeshComponent* GetFPMeshComponent() const { return FPMesh; }
	FORCEINLINE USkeletalMeshComponent* GetTPMeshComponent() const { return GetMesh(); }
};