#include "Player/ShooterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Player/ShooterPlayerCameraManager.h"

AShooterPlayerController::AShooterPlayerController()
{
	PlayerCameraManagerClass = AShooterPlayerCameraManager::StaticClass();
}

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputMappingContext, 0);
	}
}
