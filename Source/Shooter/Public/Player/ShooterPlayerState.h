// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShooterPlayerState.generated.h"

UCLASS()
class SHOOTER_API AShooterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AShooterPlayerState();

	/** Increments the kill count */
	void AddKill();

	/** Increments the death count */
	void AddDeath();

	/** Gets the current kill count */
	int32 GetKillCount() const { return KillCount; }

	/** Gets the current death count */
	int32 GetDeathCount() const { return DeathCount; }

protected:
	/** Kill count for this player */
	UPROPERTY(Replicated)
	int32 KillCount;

	/** Death count for this player */
	UPROPERTY(Replicated)
	int32 DeathCount;

	/** Setup replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};