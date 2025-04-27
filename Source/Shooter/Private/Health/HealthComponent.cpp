// HealthComponent.cpp
#include "Health/HealthComponent.h"
#include "GameFramework/Actor.h"
#include "Pickup/PickupActor.h"

UHealthComponent::UHealthComponent()
{
	DefaultHealth = 100.f;
	CurrentHealth = DefaultHealth;
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = DefaultHealth;

	if (AActor* MyOwner = GetOwner())
	{
		MyOwner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.f, DefaultHealth);
	OnHealthChanged.Broadcast(this, CurrentHealth, Damage, InstigatedBy);

	if (CurrentHealth <= 0.f)
	{
	}
}

void UHealthComponent::Heal(float Amount)
{
	CurrentHealth = FMath::Clamp(CurrentHealth - Amount, 0.f, DefaultHealth);
}

void UHealthComponent::HandlePickup_Implementation(const FPickupData& PickupData)
{
	if (PickupData.PickupType == EPickupType::Health)
	{
		Heal(PickupData.HealthAmount);
	}
}
