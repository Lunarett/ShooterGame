#include "Player/ShooterPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Debug/LoggerMacros.h"
#include "Health/HealthComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Player/ShooterCharacter.h"
#include "Player/ShooterPlayerState.h"

AShooterPlayerController::AShooterPlayerController()
{
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Triggered, this,
		                          &AShooterPlayerController::HandleMoveInput);
		EnhancedInput->BindAction(LookInputAction, ETriggerEvent::Triggered, this,
		                          &AShooterPlayerController::HandleLookInput);
		EnhancedInput->BindAction(JumpInputAction, ETriggerEvent::Triggered, this,
		                          &AShooterPlayerController::HandleJumpInput);
		EnhancedInput->BindAction(FireInputAction, ETriggerEvent::Started, this,
		                          &AShooterPlayerController::HandleBeginFireInput);
		EnhancedInput->BindAction(FireInputAction, ETriggerEvent::Completed, this,
		                          &AShooterPlayerController::HandleEndFireInput);
		EnhancedInput->BindAction(ReloadInputAction, ETriggerEvent::Started, this,
		                          &AShooterPlayerController::HandleWeaponReloadInput);
		EnhancedInput->BindAction(NextWeaponInputAction, ETriggerEvent::Started, this,
		                          &AShooterPlayerController::HandleNextWeaponInput);
		EnhancedInput->BindAction(PreviousWeaponInputAction, ETriggerEvent::Started, this,
		                          &AShooterPlayerController::HandlePreviousWeaponInput);
		EnhancedInput->BindAction(ChangeViewInputAction, ETriggerEvent::Started, this,
							      &AShooterPlayerController::HandleToggleViewModeInput);
	}
}


void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	ShooterCharacter = Cast<AShooterCharacter>(GetPawn());
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputMappingContext, 0);
	}
}

void AShooterPlayerController::OnPossess(APawn* PossessedPawn)
{
	Super::OnPossess(PossessedPawn);

	if (ShooterCharacter = Cast<AShooterCharacter>(PossessedPawn); IsValid(ShooterCharacter))	
	{
		if (UHealthComponent* HealthComponent = ShooterCharacter->FindComponentByClass<UHealthComponent>())
		{
			HealthComponent->OnDeath.AddDynamic(this, &AShooterPlayerController::HandlePawnDeath);
		}
	}
}

void AShooterPlayerController::HandleMoveInput(const FInputActionValue& Value)
{
	if (!IsValid(ShooterCharacter))
	{
		LOG_ERROR("ShooterCharacter is not valid");
		return;
	}

	const FVector2D MoveInput = Value.Get<FVector2D>();
	if (!MoveInput.IsNearlyZero())
	{
		const FRotator PlayerControlRotation = GetControlRotation();
		const FRotator YawRotation(0, PlayerControlRotation.Yaw, 0);

		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		ShooterCharacter->AddMovementInput(Forward, MoveInput.Y);
		ShooterCharacter->AddMovementInput(Right, MoveInput.X);
	}
}

void AShooterPlayerController::HandleLookInput(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	if (!LookInput.IsNearlyZero())
	{
		AddYawInput(LookInput.X);
		AddPitchInput(LookInput.Y);
		OnLookInputChanged.Broadcast(LookInput);
	}
}

void AShooterPlayerController::HandleJumpInput(const FInputActionValue& Value)
{
	if (!IsValid(ShooterCharacter))
	{
		LOG_ERROR("ShooterCharacter is not valid");
		return;
	}

	ShooterCharacter->Jump();
}

void AShooterPlayerController::HandleBeginFireInput(const FInputActionValue& Value)
{
	if (!IsValid(ShooterCharacter))
	{
		LOG_ERROR("ShooterCharacter is not valid");
		return;
	}

	if (!ShooterCharacter->GetInventoryComponent()->GetIsEquipActive())
	{
		ShooterCharacter->BeginFire();
	}
	else
	{
		ShooterCharacter->EndFire();
	}
}

void AShooterPlayerController::HandleEndFireInput(const FInputActionValue& Value)
{
	if (!IsValid(ShooterCharacter))
	{
		LOG_ERROR("ShooterCharacter is not valid");
		return;
	}

	ShooterCharacter->EndFire();
}

void AShooterPlayerController::HandleWeaponReloadInput(const FInputActionValue& Value)
{
	if (!IsValid(ShooterCharacter))
	{
		LOG_ERROR("ShooterCharacter is not valid");
		return;
	}

	if (!ShooterCharacter->GetInventoryComponent()->GetIsEquipActive())
	{
		ShooterCharacter->ReloadWeapon();
	}
}

void AShooterPlayerController::HandleNextWeaponInput(const FInputActionValue& Value)
{
	if (!IsValid(ShooterCharacter))
	{
		LOG_ERROR("ShooterCharacter is not valid");
		return;
	}

	UInventoryComponent* InventoryComponent = ShooterCharacter->GetInventoryComponent();
	if (!IsValid(InventoryComponent))
	{
		LOG_ERROR("InventoryComponent is not valid");
		return;
	}
	
	InventoryComponent->EquipNextWeapon();
}

void AShooterPlayerController::HandlePreviousWeaponInput(const FInputActionValue& Value)
{
	if (!IsValid(ShooterCharacter))
	{
		LOG_ERROR("ShooterCharacter is not valid");
		return;
	}

	UInventoryComponent* InventoryComponent = ShooterCharacter->GetInventoryComponent();
	if (!IsValid(InventoryComponent))
	{
		LOG_ERROR("InventoryComponent is not valid");
		return;
	}
	
	InventoryComponent->EquipPreviousWeapon();
}

void AShooterPlayerController::HandleToggleViewModeInput(const FInputActionValue& Value)
{
	if (!IsValid(ShooterCharacter))
	{
		LOG_ERROR("ShooterCharacter is not valid");
		return;
	}

	const EPlayerViewMode NewMode = ShooterCharacter->GetViewMode() == EPlayerViewMode::FirstPerson
		? EPlayerViewMode::ThirdPerson
		: EPlayerViewMode::FirstPerson;
	
	ShooterCharacter->SetPlayerViewMode(NewMode);
}

void AShooterPlayerController::HandlePawnDeath(AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsValid(ShooterCharacter))
	{
		return;		
	}

	// Update death count
	if (AShooterPlayerState* MyState = GetPlayerState<AShooterPlayerState>())
	{
		MyState->AddDeath();
	}

	// Award kill to instigator
	if (InstigatedBy && InstigatedBy != this)
	{
		if (APlayerState* InstigatorState = InstigatedBy->PlayerState)
		{
			if (AShooterPlayerState* ShooterState = Cast<AShooterPlayerState>(InstigatorState))
			{
				ShooterState->AddKill();
			}
		}
	}

	//BeginSpectatingState();
	
	// // Delay respawn
	// FTimerHandle RespawnTimerHandle;
	// GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, [this]()
	// {
	// 	ServerRestartPlayer();
	// }, 3.0f, false);
}
