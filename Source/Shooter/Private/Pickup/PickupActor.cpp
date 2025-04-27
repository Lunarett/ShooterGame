// Fill out your copyright notice in the Description page of Project Settings.

#include "Pickup/PickupActor.h"

#include "Components/SphereComponent.h"
#include "Pickup/PickupReceiver.h"

APickupActor::APickupActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Initialize Pickup Zone Collider
	PickupZoneSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Pickup Zone"));
	SetRootComponent(PickupZoneSphereComponent);
	PickupZoneSphereComponent->InitSphereRadius(100.0f);
	PickupZoneSphereComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PickupZoneSphereComponent->OnComponentBeginOverlap.AddDynamic(this, &APickupActor::HandleSphereOverlap);
	
	bIsCooldownActive = false;
}

void APickupActor::HandleSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsCooldownActive || !IsValid(OtherActor))
	{
		return;
	}

	bool bPickupHandled = false;

	// Execute interface if Actor implements one
	if (OtherActor->GetClass()->ImplementsInterface(UPickupReceiver::StaticClass()))
	{
		IPickupReceiver::Execute_HandlePickup(OtherActor, PickupData);
		OnPickupCooldownBegin();
		bPickupHandled = true;
	}

	// Iterate through Actor's components and execute interface if they implement one
	TArray<UActorComponent*> Components = OtherActor->GetComponents().Array();
	for (UActorComponent* Component : Components)
	{
		if (Component->GetClass()->ImplementsInterface(UPickupReceiver::StaticClass()))
		{
			IPickupReceiver::Execute_HandlePickup(Component, PickupData);
			bPickupHandled = true;
		}
	}

	// Execute Cooldown if Interface was executed
	if (bPickupHandled)
	{
		OnPickupCooldownBegin();
	}
}

void APickupActor::OnPickupCooldownBegin_Implementation()
{
	bIsCooldownActive = true;

	// Start the cooldown timer
	GetWorld()->GetTimerManager().SetTimer(
		PickupCooldownTimer,
		[this]()
		{
			this->OnPickupCooldownEnd();
		},
		PickupCoolDownDuration,
		false
	);
}

void APickupActor::OnPickupCooldownEnd_Implementation()
{
	bIsCooldownActive = false;
}
