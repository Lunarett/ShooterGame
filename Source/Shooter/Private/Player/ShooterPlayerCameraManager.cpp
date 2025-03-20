#include "Player/ShooterPlayerCameraManager.h"
#include "Player/ShooterCharacter.h"

AShooterPlayerCameraManager::AShooterPlayerCameraManager()
{
	InitialFOV = 90.0f;
	FOVSpeed = 20.0f;
	ViewPitchMin = -87.0f;
	ViewPitchMax = 87.0f;
	
	bAlwaysApplyModifiers = true;
}

void AShooterPlayerCameraManager::BeginPlay()
{
	Super::BeginPlay();

	if (PCOwner && PCOwner->GetPawn())
	{
		ShooterCharacter = Cast<AShooterCharacter>(PCOwner->GetPawn());
	}

	TargetFOV = InitialFOV;
}

void AShooterPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	UpdateCameraFOV(DeltaTime);
	
	Super::UpdateCamera(DeltaTime);
}

void AShooterPlayerCameraManager::UpdateCameraFOV(float DeltaTime)
{
	if (!ShooterCharacter)
	{
		return;
	}

	DefaultFOV = FMath::FInterpTo(DefaultFOV, TargetFOV, DeltaTime, FOVSpeed);
}

void AShooterPlayerCameraManager::SetCameraFOV(float InFOV)
{
	TargetFOV = InFOV;
}

void AShooterPlayerCameraManager::SetCameraFOVSpeed(float InFOVSpeed)
{
	FOVSpeed = InFOVSpeed;
}
