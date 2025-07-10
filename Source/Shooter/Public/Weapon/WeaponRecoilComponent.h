#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "WeaponRecoilComponent.generated.h"

class UCurveFloat;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTER_API UWeaponRecoilComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponRecoilComponent();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Strength")
	bool bEnableRecoil;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Strength")
	float MinRecoilVerticalStrength;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Strength")
	float MaxRecoilVerticalStrength;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Strength")
	bool bForceMinMaxVerticalStrength;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Strength")
	float MinRecoilHorizontalStrength;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Strength")
	float MaxRecoilHorizontalStrength;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Strength")
	bool bForceMinMaxHorizontalStrength;
	

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Scale")
	float RecoilScale;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Scale")
	float RecoilScaleADS;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Scale")
	float RecoilScaleCrouch;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Scale")
	float RecoilScaleADSCrouch;
	

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Animation")
	float RecoilSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Animation")
	TEnumAsByte<EEasingFunc::Type> RecoilInterpolation;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Animation")
	float RecoilInterpolationEaseExp;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Animation")
	int32 RecoilInterpolationSteps;


	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Reset")
	bool bResetRecoil;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Reset")
	float RecoilResetDelay;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Reset")
	float RecoilResetSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Reset")
	TEnumAsByte<EEasingFunc::Type> RecoilResetInterpolation;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Reset")
	float RecoilResetInterpolationEaseExp;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Reset")
	float RecoilResetInterpolationSteps;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Curves")
	UCurveFloat* AddRecoilCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil|Curves")
	UCurveFloat* ResetRecoilCurve;

private:
	bool bIsADS;
	
	FTimeline AddRecoilTimeline;
	FTimeline ResetRecoilTimeline;

	float CurrentYaw;
	float CurrentPitch;
	
	float TempAddedYaw;
	float TempAddedPitch;
	
	FVector2D AddedPitchAndYaw;
	FVector2D TempAddedPitchAndYaw;

	int32 TempRecoilPosition;
	int32 CurrentRecoilPosition;

	float TempRecoilResetPitchOffset;

	FTimerHandle RecoilResetTimerHandle;

	float ResetEasedYawTotal = 0.0f;
	float ResetEasedPitchTotal = 0.0f;

	float AddEasedYawTotal = 0.0f;
	float AddEasedPitchTotal = 0.0f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	void AddRecoil();
	void ResetRecoil();
	void ResetRecoilState();

	UFUNCTION()
	void OnYawAdded(const float InAxisValue);

	UFUNCTION()
	void OnPitchAdded(const float InAxisValue);

private:
	void GetRecoilYawAndPitch(float& OutYaw, float& OutPitch);
	void UpdatePlayerYawAndPitch(const float InYaw, const float InPitch);
	
	UFUNCTION()
	void OnTimelineAddRecoilUpdate(const float InAlpha);

	UFUNCTION()
	void OnTimelineAddRecoilComplete();

	UFUNCTION()
	void OnTimelineResetRecoilUpdate(const float InAlpha);
};