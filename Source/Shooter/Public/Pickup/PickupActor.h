#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "PickupActor.generated.h"

class USphereComponent;
class AWeaponBase;

UENUM(BlueprintType)
enum class EPickupType : uint8
{
	Weapon,
	Ammo,
	Health,

	// Continue the list here as more "Pickup" items come...
};

USTRUCT(BlueprintType)
struct FPickupData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	EPickupType PickupType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup",
		meta = (EditCondition = "PickupType == EPickupType::Weapon", EditConditionHides))
	TSubclassOf<AWeaponBase> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup",
		meta = (EditCondition = "PickupType == EPickupType::Ammo", EditConditionHides))
	FGameplayTag AmmoTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup",
		meta = (EditCondition = "PickupType == EPickupType::Ammo", EditConditionHides))
	int32 AmmoAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup",
		meta = (EditCondition = "PickupType == EPickupType::Health", EditConditionHides))
	float HealthAmount;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPickupCooldownBegin);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPickupCooldownEnd);

UCLASS(Abstract)
class SHOOTER_API APickupActor : public AActor
{
	GENERATED_BODY()

public:
	APickupActor();

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (AllowPrivateAccess = "true"))
	USphereComponent* PickupZoneSphereComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (AllowPrivateAccess = "true"))
	FPickupData PickupData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (AllowPrivateAccess = "true"))
	float PickupCoolDownDuration;

private:
	bool bIsCooldownActive;
	FTimerHandle PickupCooldownTimer;

protected:
	UFUNCTION()
	void HandleSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                         const FHitResult& SweepResult);

protected:
	/* Override this method in your child class and implement your logic to *HIDE* the display mesh */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pickup")
	void OnPickupCooldownBegin();

	/* Override this method in your child class and implement your logic to *SHOW* the display mesh */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pickup")
	void OnPickupCooldownEnd();
};
