// ShooterCharacter.h

/**
 * Player controlled character used in Shooter. Handles weapon management,
 * first/third person camera switching and replication of relevant state.
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Weapon/AmmoProvider.h"
#include "ShooterCharacter.generated.h"


// Possible camera perspectives for the player
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
class UHealthComponent;

UCLASS()
/** Main player character class. Handles first/third person switching and weapon logic. */
class SHOOTER_API AShooterCharacter : public ACharacter, public IAmmoProvider
{
        GENERATED_BODY()

public:
        AShooterCharacter();

protected:
       // --------------------------------------------------------------------
       // Components
       // --------------------------------------------------------------------
       /** Spring arm used to position the first person mesh */
       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shooter Character|Components")
       USpringArmComponent* FPMeshRootSpringArmComponent;

       /** First person body mesh */
       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shooter Character|Components")
       USkeletalMeshComponent* FPMesh;

       /** Spring arm used for the first person camera */
       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shooter Character|Components")
       USpringArmComponent* FPCameraRootSpringArmComponent;

       /** Mesh used to attach the camera for animations */
       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shooter Character|Components")
       USkeletalMeshComponent* CameraSkeletalMesh;

       /** Player camera component */
       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shooter Character|Components")
       UCameraComponent* CameraComponent;

       /** Third person camera spring arm */
       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shooter Character|Components")
       USpringArmComponent* TPSpringArmComponent;

       /** Manages health and damage */
       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shooter Character|Components")
       UHealthComponent* HealthComponent;

       /** Handles weapons and ammo */
       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shooter Character|Components")
       UInventoryComponent* InventoryComponent;


protected:
       // --------------------------------------------------------------------
       // Configuration
       // --------------------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Weapon")
	FName WeaponSocketName;
	
	UPROPERTY(ReplicatedUsing=OnRep_WeaponActor, BlueprintReadOnly, Category = "Shooter Character|Weapon")
	AWeaponBase* WeaponActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character|Animations")
	UAnimMontage* EquipMontage;

       /** Current camera mode */
       UPROPERTY(ReplicatedUsing = OnRep_ViewMode, EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter Character")
       EPlayerViewMode ViewMode;
	

private:
       // --------------------------------------------------------------------
       // Replicated state
       // --------------------------------------------------------------------
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

public:
	//virtual void Jump() override;

protected:
	//virtual void Landed(const FHitResult& Hit) override;
	
private:
	UFUNCTION()
	void OnWeaponEquipped(float EquipCooldownDuration);
	
public:
	UFUNCTION(BlueprintCallable)
	void Kill();
	
	UFUNCTION(BlueprintCallable)
	void BeginFire();

	UFUNCTION(BlueprintCallable)
	void EndFire();

	UFUNCTION(BlueprintCallable)
	void ReloadWeapon();

	virtual int32 RequestAmmo_Implementation(FGameplayTag AmmoType, int32 RequestedAmount) override;

       /** Requests a view mode change. Will be replicated to all clients. */
       void SetPlayerViewMode(EPlayerViewMode NewViewMode);

private:
       /** Applies the current view mode locally on this instance */
       void ApplyViewMode();

       UFUNCTION(Server, Reliable)
       void ServerSetPlayerViewMode(EPlayerViewMode NewViewMode);

       UFUNCTION()
       /** Called when ViewMode is replicated */
       void OnRep_ViewMode();

       void AttachWeaponToMesh();
       void PlayEquipMontage(bool bReverse);

       UFUNCTION()
        /** Called when the equipped weapon changes */
        void OnRep_WeaponActor();

       UFUNCTION(NetMulticast, Reliable)
       void HandleCharacterDeath(AController* InstigatedBy, AActor* DamageCauser);

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