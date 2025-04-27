// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/BobFloatMovementComponent.h"

UBobFloatMovementComponent::UBobFloatMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	Amplitude = 10.0f;
	Frequency = 1.0f;
}

void UBobFloatMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	InitialLocation = UpdatedComponent->GetComponentLocation();
}

void UBobFloatMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(UpdatedComponent))
	{
		return;
	}

	RunningTime += DeltaTime;

	const float BobOffset = FMath::Sin(RunningTime * Frequency) * Amplitude;

	FVector NewLocation = InitialLocation;
	NewLocation.Z += BobOffset;

	UpdatedComponent->SetWorldLocation(NewLocation);
}
