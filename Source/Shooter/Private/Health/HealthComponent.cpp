#include "Health/HealthComponent.h"
#include "GameFramework/Actor.h"
#include "Pickup/PickupActor.h"

UHealthComponent::UHealthComponent()
{
	MaxHealth = 100.f;
	CurrentHealth = MaxHealth;
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;

	if (AActor* MyOwner = GetOwner())
	{
		MyOwner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, const float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, InstigatedBy, DamageCauser);

	if (CurrentHealth <= 0.0f)
	{
		OnDeath.Broadcast(InstigatedBy, DamageCauser);
	}
}

void UHealthComponent::Heal(const float Amount)
{
	CurrentHealth = FMath::Clamp(CurrentHealth - Amount, 0.f, MaxHealth);
}

void UHealthComponent::HandlePickup_Implementation(const FPickupData& PickupData)
{
	if (PickupData.PickupType == EPickupType::Health)
	{
		Heal(PickupData.HealthAmount);
	}
}
