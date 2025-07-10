#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Pickup/PickupReceiver.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChangedSignature, float, NewHealth, AController*, InstigatedBy, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeathSignature, AController*, InstigatedBy, AActor*, DamageCauser);

/*
 * A health system component that handles damage, healing, and death events.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTER_API UHealthComponent : public UActorComponent, public IPickupReceiver
{
	GENERATED_BODY()

public:	
	UHealthComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Health")
	float MaxHealth;

private:
	float CurrentHealth;
	bool bIsDead;

public:
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnDeathSignature OnDeath;

protected:
	virtual void BeginPlay() override;

private:
	/* Responds to any damage the owning actor takes */
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

public:
	/* Increases the actor's health by the given amount (clamped to max health) */
	void Heal(float Amount);
	
private:
	/* Applies health-related effects when picking up a health item */
	virtual void HandlePickup_Implementation(const FPickupData& PickupData) override;

public:
	FORCEINLINE bool IsDead() const { return bIsDead; }
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
};