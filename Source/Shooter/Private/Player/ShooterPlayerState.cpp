#include "Player/ShooterPlayerState.h"
#include "Net/UnrealNetwork.h"

AShooterPlayerState::AShooterPlayerState()
{
	KillCount = 0;
	DeathCount = 0;
}

void AShooterPlayerState::AddKill()
{
	++KillCount;
}

void AShooterPlayerState::AddDeath()
{
	++DeathCount;
}

void AShooterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerState, KillCount);
	DOREPLIFETIME(AShooterPlayerState, DeathCount);
}