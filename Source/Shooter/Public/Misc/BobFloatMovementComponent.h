// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "BobFloatMovementComponent.generated.h"

/*
 * This class is just for cosmetic purposes
 * Use this class if you want something to bob up and down
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTER_API UBobFloatMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
	UBobFloatMovementComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bob Behavior")
	float Amplitude;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bob Behavior")
	float Frequency;

private:
	FVector InitialLocation;
	float RunningTime = 0.0f;

protected:
	virtual void BeginPlay() override;
	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};