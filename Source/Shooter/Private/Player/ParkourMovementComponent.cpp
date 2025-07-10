#include "Player/ParkourMovementComponent.h"
#include "Debug/LoggerMacros.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UParkourMovementComponent::UParkourMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	UpdateRate = 0.02f;

	ActorWallTag = FName("Wall");
	TraceDistance = 125.0f;
	TraceAngle = -45.0f;

	bUseGravity = true;
	WallRunGravity = 0.0f;
}

void UParkourMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		LOG_ERROR_SCREEN("Owner is invalid!");
		return;
	}

	OwnerCharacter = Cast<ACharacter>(OwnerActor);
	if (!IsValid(OwnerCharacter))
	{
		LOG_ERROR_SCREEN("Cast to Character failed! Parkour Component can only be used on Characters!");
		return;
	}

	InitialGravityScale = OwnerCharacter->GetCharacterMovement()->GravityScale;

	GetWorld()->GetTimerManager().SetTimer(UpdateTimerHandle, this, &UParkourMovementComponent::UpdateParkourMovement,
	                                       UpdateRate, true);
}

void UParkourMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsWallRunningLeft)
	{
		CameraTilt(15.0f);
	}
	else if (bIsWallRunningRight)
	{
		CameraTilt(-15);
	}
	else
	{
		CameraTilt(0.0f);
	}
}

void UParkourMovementComponent::UpdateParkourMovement()
{
	if (bIsWallSuppressed)
	{
		return;
	}

	UpdateWallRun();
}

void UParkourMovementComponent::UpdateWallRun()
{
	FVector Left, Right;
	GetWallRunEndVectors(Left, Right);

	if (HandleWallRunning(OwnerCharacter->GetActorLocation(), Right, -1.0f))
	{
		bIsWallRunning = true;
		bIsWallRunningLeft = false;
		bIsWallRunningRight = true;
	}
	else if (bIsWallRunningRight)
	{
		WallRunEnd(1.0f);
	}
	else
	{
		if (HandleWallRunning(OwnerCharacter->GetActorLocation(), Left, 1.0f))
		{
			bIsWallRunning = true;
			bIsWallRunningLeft = true;
			bIsWallRunningRight = false;
		}
		else
		{
			WallRunEnd(-1.0f);
		}
	}

	const float InterpolatedGravityScale = UKismetMathLibrary::FInterpTo(
		OwnerCharacter->GetCharacterMovement()->GravityScale,
		WallRunGravity, GetWorld()->GetDeltaSeconds(), 0.01f);

	OwnerCharacter->GetCharacterMovement()->GravityScale = InterpolatedGravityScale;
}

bool UParkourMovementComponent::HandleWallRunning(const FVector& Start, const FVector& End, const float Direction)
{
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams))
	{
		const bool bIsFalling = OwnerCharacter->GetCharacterMovement()->IsFalling();

		// Climb only on actors that have the tag
		if (HitResult.GetActor()->ActorHasTag(ActorWallTag) && bIsFalling &&
			IsValidImpactNormal(HitResult.ImpactNormal))
		{
			OwnerCharacter->GetCharacterMovement()->GravityScale = WallRunGravity;
			WallRunNormal = HitResult.ImpactNormal;

			const FVector StickToWallVelocity = CalculateStickToWallVelocity();
			const FVector ForwardVelocity = (FVector::CrossProduct(WallRunNormal, FVector(0.0f, 0.0f, 1.0f)) * (
				WallRunSpeed * Direction));

			OwnerCharacter->LaunchCharacter(StickToWallVelocity, false, false);
			OwnerCharacter->LaunchCharacter(ForwardVelocity, true, !bUseGravity);

			return true;
		}
	}

	return false;
}

void UParkourMovementComponent::WallRunEnd(float ResetTimer)
{
	if (!bIsWallRunning)
	{
		return;
	}

	bIsWallRunning = false;
	bIsWallRunningRight = false;
	bIsWallRunningLeft = false;

	OwnerCharacter->GetCharacterMovement()->GravityScale = InitialGravityScale;
	SuppressWallRunning(ResetTimer);
}

void UParkourMovementComponent::SuppressWallRunning(const float Delay)
{
	bIsWallSuppressed = true;

	GetWorld()->GetTimerManager().SetTimer(WallRunSuppressTimerHandle, [this]()
	                                       {
		                                       GetWorld()->GetTimerManager().ClearTimer(WallRunSuppressTimerHandle);
		                                       bIsWallSuppressed = false;
	                                       },
	                                       Delay,
	                                       false);
}

void UParkourMovementComponent::CameraTilt(float Roll)
{
	const FRotator ControlRotation = OwnerCharacter->GetController()->GetControlRotation();
	const FRotator TargetRotation = FRotator(ControlRotation.Pitch, ControlRotation.Yaw, Roll);
	const FRotator TiltRotation = UKismetMathLibrary::RInterpTo(OwnerCharacter->GetController()->GetControlRotation(),
	                                                            TargetRotation, GetWorld()->GetDeltaSeconds(), 10.0f);

	OwnerCharacter->GetController()->SetControlRotation(TiltRotation);
}

void UParkourMovementComponent::WallRunLand()
{
	WallRunEnd(0.0f);
	bIsWallSuppressed = false;
}

void UParkourMovementComponent::WallRunJump()
{
	if (!bIsWallRunning)
	{
		return;
	}

	WallRunEnd(0.35f);

	const FVector LaunchVelocity = FVector(WallRunNormal.X * WallRunJumpOffForce, WallRunNormal.Y * WallRunJumpOffForce,
	                                       WallRunJumpHeight);

	OwnerCharacter->LaunchCharacter(LaunchVelocity, false, true);
}

void UParkourMovementComponent::GetWallRunEndVectors(FVector& Left, FVector& Right)
{
	if (!IsValid(OwnerCharacter))
	{
		return;
	}

	const FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	const FVector OwnerRightVector = OwnerCharacter->GetActorRightVector();
	const FVector OwnerForwardVector = OwnerCharacter->GetActorForwardVector();

	Right = OwnerLocation + (OwnerRightVector * TraceDistance) + (OwnerForwardVector * TraceAngle);
	Left = OwnerLocation + (OwnerRightVector * -TraceDistance) - (OwnerForwardVector * TraceAngle);
}

bool UParkourMovementComponent::IsValidImpactNormal(const FVector& ImpactNormal)
{
	return UKismetMathLibrary::InRange_FloatFloat(ImpactNormal.Z, -0.52f, 0.52f, false, false);
}

FVector UParkourMovementComponent::CalculateStickToWallVelocity()
{
	const FVector Location = OwnerCharacter->GetActorLocation();
	const FVector Combined = Location + WallRunNormal;
	const float Length = Combined.Length();

	return WallRunNormal * Length;
}
