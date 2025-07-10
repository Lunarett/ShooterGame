#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

struct FInputActionValue;
class AShooterCharacter;
class UInputAction;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLookInputChanged, FVector2D, MouseDelta);

UCLASS()
class SHOOTER_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AShooterPlayerController();

private:
	UPROPERTY()
	AShooterCharacter* ShooterCharacter;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* InputMappingContext;

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* NextWeaponInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* PreviousWeaponInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* FireInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ChangeViewInputAction;

private:
	virtual void SetupInputComponent() override;
	
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* PossessedPawn) override;

private:
	/* Handle Player Input */
	void HandleMoveInput(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void HandleJumpInput(const FInputActionValue& Value);
	void HandleBeginFireInput(const FInputActionValue& Value);
	void HandleEndFireInput(const FInputActionValue& Value);
	void HandleWeaponReloadInput(const FInputActionValue& Value);
	void HandleNextWeaponInput(const FInputActionValue& Value);
	void HandlePreviousWeaponInput(const FInputActionValue& Value);
	void HandleToggleViewModeInput(const FInputActionValue& Value);

	UFUNCTION()
	void HandlePawnDeath(AController* InstigatedBy, AActor* DamageCauser);

public:
	FORCEINLINE AShooterCharacter* GetShooterCharacter() const { return ShooterCharacter; }
	
	UPROPERTY(BlueprintAssignable)
	FOnLookInputChanged OnLookInputChanged;
};