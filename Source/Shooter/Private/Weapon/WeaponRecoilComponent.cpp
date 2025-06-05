#include "Weapon/WeaponRecoilComponent.h"
#include "Components/TimelineComponent.h"
#include "Debug/LoggerMacros.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

UWeaponRecoilComponent::UWeaponRecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	bEnableRecoil = true;
	MinRecoilVerticalStrength = 1.0f;
	MaxRecoilVerticalStrength = 1.0f;
	MinRecoilHorizontalStrength = -0.15f;
	MaxRecoilHorizontalStrength = 0.15f;

	RecoilScale = 1.0f;
	RecoilScaleADS = 0.5f;
	RecoilScaleCrouch = 0.5f;
	RecoilScaleADSCrouch = 0.25f;

	RecoilSpeed = 3.0f;
	RecoilInterpolation = EEasingFunc::EaseOut;
	RecoilInterpolationEaseExp = 2.0f;
	RecoilInterpolationSteps = 2;

	bResetRecoil = true;
	RecoilResetDelay = 0.05f;
	RecoilResetSpeed = 0.5f;
	RecoilResetInterpolation = EEasingFunc::EaseInOut;
	RecoilResetInterpolationEaseExp = 2.0f;
	RecoilResetInterpolationSteps = 2;
}

void UWeaponRecoilComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize AddRecoilTimeline
	if (AddRecoilCurve)
	{
		const float LastKeyTime = AddRecoilCurve->FloatCurve.GetLastKey().Time;

		FOnTimelineFloat OnUpdate;
		OnUpdate.BindUFunction(this, FName("OnTimelineAddRecoilUpdate"));

		FOnTimelineEvent OnFinished;
		OnFinished.BindUFunction(this, FName("OnTimelineAddRecoilComplete"));

		AddRecoilTimeline.AddInterpFloat(AddRecoilCurve, OnUpdate);
		AddRecoilTimeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
		AddRecoilTimeline.SetTimelineLength(LastKeyTime);
		AddRecoilTimeline.SetTimelineFinishedFunc(OnFinished);
		AddRecoilTimeline.SetLooping(false);
		AddRecoilTimeline.SetPlayRate(1.0f);
	}

	// Initialize ResetRecoilTimeline
	if (ResetRecoilCurve)
	{
		const float LastKeyTime = ResetRecoilCurve->FloatCurve.GetLastKey().Time;

		FOnTimelineFloat OnUpdate;
		OnUpdate.BindUFunction(this, FName("OnTimelineResetRecoilUpdate"));

		ResetRecoilTimeline.AddInterpFloat(ResetRecoilCurve, OnUpdate);
		ResetRecoilTimeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
		ResetRecoilTimeline.SetTimelineLength(LastKeyTime);
		ResetRecoilTimeline.SetLooping(false);
		ResetRecoilTimeline.SetPlayRate(1.0f);
	}
}

void UWeaponRecoilComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Tick Timeline
	AddRecoilTimeline.TickTimeline(DeltaTime);
	ResetRecoilTimeline.TickTimeline(DeltaTime);
}

void UWeaponRecoilComponent::AddRecoil()
{
	if (!bEnableRecoil)
	{
		LOG_WARN_SCREEN("Recoil Isnt enabled");
		return;
	}

	GetRecoilYawAndPitch(CurrentYaw, CurrentPitch);

	// Stop all timelines
	AddRecoilTimeline.Stop();
	ResetRecoilTimeline.Stop();

	TempRecoilResetPitchOffset = 0.0f;
	TempAddedYaw = 0.0f;
	TempAddedPitch = 0.0f;

	AddRecoilTimeline.SetPlayRate(RecoilSpeed * 5.0f);
	AddRecoilTimeline.PlayFromStart();
}

void UWeaponRecoilComponent::ResetRecoil()
{
	if (!bResetRecoil && AddRecoilTimeline.IsPlaying())
	{
		return;
	}

	ResetRecoilTimeline.Stop();

	TempRecoilPosition = CurrentRecoilPosition;
	TempAddedYaw = 0.0f;
	TempAddedPitch = 0.0f;
	TempAddedPitchAndYaw = AddedPitchAndYaw;

	ResetRecoilTimeline.SetPlayRate(RecoilResetSpeed * 5.0f);
	ResetRecoilTimeline.PlayFromStart();
}

void UWeaponRecoilComponent::ResetRecoilState()
{
	AddRecoilTimeline.Stop();
	ResetRecoilTimeline.Stop();

	CurrentRecoilPosition = 0;
	AddedPitchAndYaw = FVector2D::ZeroVector;
	TempAddedPitchAndYaw = FVector2D::ZeroVector;
}

void UWeaponRecoilComponent::OnYawAdded(const float InAxisValue)
{
	if (InAxisValue <= 0.0f)
	{
		return;
	}

	AddedPitchAndYaw.X = FMath::FInterpTo(AddedPitchAndYaw.X, 0.0f, GetWorld()->GetDeltaSeconds(), 10.0f);
}

void UWeaponRecoilComponent::OnPitchAdded(const float InAxisValue)
{
	if (InAxisValue > 0.0f)
	{
		AddedPitchAndYaw.Y = FMath::Min(AddedPitchAndYaw.Y + InAxisValue, 0.0f); 
	}

	if (ResetRecoilTimeline.IsPlaying() && InAxisValue < 0.0f)
	{
		TempRecoilResetPitchOffset += FMath::Abs(InAxisValue);

		if (TempRecoilResetPitchOffset >= (FMath::Abs(AddedPitchAndYaw.X) * 1.1f))
		{
			ResetRecoilState();
			TempRecoilResetPitchOffset = 0.0f;
		}
	}
}

void UWeaponRecoilComponent::GetRecoilYawAndPitch(float& OutYaw, float& OutPitch)
{
	float NewScale = 0.0f;
	float YawStrength = 0.0f;
	float PitchStrength = 0.0f;
	
	if (const ACharacter* Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0); IsValid(Character))
	{
		const float ScaleA = bIsADS ? RecoilScaleADSCrouch : RecoilScaleCrouch;
		const float ScaleB = bIsADS ? RecoilScaleADS : RecoilScale;
		const bool IsCrouching = Character->GetCharacterMovement()->IsCrouching();
		NewScale = IsCrouching ? ScaleA : ScaleB;
	}

	const float VStrengthA = FMath::RandBool() ? MinRecoilVerticalStrength : MaxRecoilVerticalStrength;
	const float VStrengthB = FMath::RandRange(MinRecoilVerticalStrength, MaxRecoilVerticalStrength);
	PitchStrength = bForceMinMaxVerticalStrength ? VStrengthA : VStrengthB;

	const float HStrengthA = FMath::RandBool() ? MinRecoilHorizontalStrength : MaxRecoilHorizontalStrength;
	const float HStrengthB = FMath::RandRange(MinRecoilHorizontalStrength, MaxRecoilHorizontalStrength);
	YawStrength = bForceMinMaxVerticalStrength ? HStrengthA : HStrengthB;

	OutPitch = (PitchStrength * -1.0f) * NewScale;
	OutYaw = YawStrength * NewScale;
}

void UWeaponRecoilComponent::UpdatePlayerYawAndPitch(const float InYaw, const float InPitch)
{
	if (ACharacter* Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0); IsValid(Character))
	{
		Character->AddControllerYawInput(InYaw);
		Character->AddControllerPitchInput(InPitch);
	}
}

void UWeaponRecoilComponent::OnTimelineAddRecoilUpdate(const float InAlpha)
{
	const double EasedYaw = UKismetMathLibrary::Ease(0.0f, CurrentYaw, InAlpha, RecoilInterpolation,
	                                                 RecoilInterpolationEaseExp,
	                                                 RecoilInterpolationSteps) - TempAddedYaw;

	const double EasedPitch = UKismetMathLibrary::Ease(0.0f, CurrentPitch, InAlpha, RecoilInterpolation,
	                                                   RecoilInterpolationEaseExp,
	                                                   RecoilInterpolationSteps) - TempAddedPitch;

	UpdatePlayerYawAndPitch(EasedYaw, EasedPitch);

	AddedPitchAndYaw += FVector2D(EasedYaw, EasedPitch);
	TempAddedPitch = EasedPitch;
	TempAddedYaw = EasedYaw;
}

void UWeaponRecoilComponent::OnTimelineAddRecoilComplete()
{
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		GetWorld()->GetTimerManager().SetTimer(
			RecoilResetTimerHandle,
			[this]()
			{
				ResetRecoil();
			},
			RecoilResetDelay,
			false
		);
	});
}

void UWeaponRecoilComponent::OnTimelineResetRecoilUpdate(const float InAlpha)
{
	const double EasedYaw = (UKismetMathLibrary::Ease(0.0f, AddedPitchAndYaw.X, InAlpha, RecoilResetInterpolation,
	                                                  RecoilResetInterpolationEaseExp,
	                                                  RecoilResetInterpolationSteps) - TempAddedYaw) * -1.0f;

	const double EasedPitch = (UKismetMathLibrary::Ease(0.0f, AddedPitchAndYaw.Y, InAlpha, RecoilResetInterpolation,
	                                                    RecoilResetInterpolationEaseExp,
	                                                    RecoilResetInterpolationSteps) - TempAddedPitch) * -1.0f;


	UpdatePlayerYawAndPitch(EasedYaw, EasedPitch);
	AddedPitchAndYaw += FVector2D(EasedYaw, EasedPitch);
	TempAddedPitch = EasedPitch;
	TempAddedYaw = EasedYaw;

	CurrentRecoilPosition = FMath::TruncToInt(FMath::Lerp(TempRecoilPosition, 0.0f, InAlpha));
}