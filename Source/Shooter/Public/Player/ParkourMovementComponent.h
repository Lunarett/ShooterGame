// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "ParkourMovementComponent.generated.h"

class ACharacter;

UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class SHOOTER_API UParkourMovementComponent : public UCharacterMovementComponent
{
        GENERATED_BODY()

public:
       UParkourMovementComponent();

       virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
       virtual void UpdateFromCompressedFlags(uint8 Flags) override;
       virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Parkour Movement")
	float UpdateRate;

	UPROPERTY(EditDefaultsOnly, Category = "Parkour Movement|Wall Run")
	FName ActorWallTag;

	UPROPERTY(EditDefaultsOnly, Category = "Parkour Movement|Wall Run|Detection Trace")
	float TraceDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Parkour Movement|Wall Run|Detection Trace")
	float TraceAngle;

	UPROPERTY(EditDefaultsOnly, Category = "Parkour Movement|Wall Run")
	bool bUseGravity;

	UPROPERTY(EditDefaultsOnly, Category = "Parkour Movement|Wall Run")
	float WallRunGravity;

	UPROPERTY(EditDefaultsOnly, Category = "Parkour Movement|Wall Run")
	float WallRunSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Parkour Movement|Wall Run")
	float WallRunJumpOffForce;

	UPROPERTY(EditDefaultsOnly, Category = "Parkour Movement|Wall Run")
	float WallRunJumpHeight;

private:

       UPROPERTY(Replicated)
       bool bIsWallRunning;

       UPROPERTY(Replicated)
       bool bIsWallRunningLeft;

       UPROPERTY(Replicated)
       bool bIsWallRunningRight;

       UPROPERTY(Replicated)
       bool bIsWallSuppressed;

       UPROPERTY(Replicated)
       float InitialGravityScale;

       UPROPERTY(Replicated)
       FVector WallRunNormal;

	FTimerHandle UpdateTimerHandle;
	FTimerHandle WallRunSuppressTimerHandle;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateParkourMovement();

	void UpdateWallRun();
	bool HandleWallRunning(const FVector& Start, const FVector& End, const float Direction);
	void SuppressWallRunning(const float Delay);

	void CameraTilt(float Roll);

public:
	UFUNCTION(BlueprintCallable)
	void WallRunEnd(float ResetTimer);
	
	UFUNCTION(blueprintCallable)
	void WallRunLand();

	UFUNCTION(blueprintCallable)
	void WallRunJump();

private:
	void GetWallRunEndVectors(FVector& Left, FVector& Right);
	bool IsValidImpactNormal(const FVector& ImpactNormal);
	FVector CalculateStickToWallVelocity();

public:
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool GetIsWallRunningLeft() const { return bIsWallRunningLeft; }

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool GetIsWallRunningRight() const { return bIsWallRunningRight; }

	UFUNCTION(BlueprintCallable)
        FORCEINLINE bool GetIsWallRunning() const { return bIsWallRunning; }
};

// Saved move for predicting wall run state
class FSavedMove_Parkour : public FSavedMove_Character
{
public:
    typedef FSavedMove_Character Super;

    bool bSavedIsWallRunning = false;
    bool bSavedIsWallRunningLeft = false;
    bool bSavedIsWallRunningRight = false;
    bool bSavedIsWallSuppressed = false;

    virtual void Clear() override;
    virtual uint8 GetCompressedFlags() const override;
    virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
    virtual void PrepMoveFor(ACharacter* Character) override;
};

class FNetworkPredictionData_Client_Parkour : public FNetworkPredictionData_Client_Character
{
public:
    FNetworkPredictionData_Client_Parkour(const UCharacterMovementComponent& ClientMovement);

    virtual FSavedMovePtr AllocateNewMove() override;
};
