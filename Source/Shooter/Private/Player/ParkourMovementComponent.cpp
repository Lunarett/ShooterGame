#include "Player/ParkourMovementComponent.h"
#include "Debug/LoggerMacros.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

UParkourMovementComponent::UParkourMovementComponent()
{
        PrimaryComponentTick.bCanEverTick = true;
        SetIsReplicatedByDefault(true);

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

    if (!CharacterOwner)
    {
            LOG_ERROR_SCREEN("ParkourMovementComponent has no character owner!");
            return;
    }

    InitialGravityScale = CharacterOwner->GetCharacterMovement()->GravityScale;

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

    if (HandleWallRunning(CharacterOwner->GetActorLocation(), Right, -1.0f))
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
            if (HandleWallRunning(CharacterOwner->GetActorLocation(), Left, 1.0f))
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
                GravityScale,
                WallRunGravity, GetWorld()->GetDeltaSeconds(), 0.01f);

        GravityScale = InterpolatedGravityScale;
}

bool UParkourMovementComponent::HandleWallRunning(const FVector& Start, const FVector& End, const float Direction)
{
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams))
	{
                const bool bIsFalling = IsFalling();

		// Climb only on actors that have the tag
		if (HitResult.GetActor()->ActorHasTag(ActorWallTag) && bIsFalling &&
			IsValidImpactNormal(HitResult.ImpactNormal))
		{
                        GravityScale = WallRunGravity;
			WallRunNormal = HitResult.ImpactNormal;

			const FVector StickToWallVelocity = CalculateStickToWallVelocity();
			const FVector ForwardVelocity = (FVector::CrossProduct(WallRunNormal, FVector(0.0f, 0.0f, 1.0f)) * (
				WallRunSpeed * Direction));

                        CharacterOwner->LaunchCharacter(StickToWallVelocity, false, false);
                        CharacterOwner->LaunchCharacter(ForwardVelocity, true, !bUseGravity);

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

    GravityScale = InitialGravityScale;
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
    if (!IsValid(CharacterOwner) || !IsValid(CharacterOwner->GetController()))
	{
		return;
	}
	
    const FRotator ControlRotation = CharacterOwner->GetController()->GetControlRotation();
	const FRotator TargetRotation = FRotator(ControlRotation.Pitch, ControlRotation.Yaw, Roll);
    const FRotator TiltRotation = UKismetMathLibrary::RInterpTo(CharacterOwner->GetController()->GetControlRotation(),
	                                                            TargetRotation, GetWorld()->GetDeltaSeconds(), 10.0f);

    CharacterOwner->GetController()->SetControlRotation(TiltRotation);
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

    CharacterOwner->LaunchCharacter(LaunchVelocity, false, true);
}

void UParkourMovementComponent::GetWallRunEndVectors(FVector& Left, FVector& Right)
{
    if (!IsValid(CharacterOwner))
	{
		return;
	}

    const FVector OwnerLocation = CharacterOwner->GetActorLocation();
    const FVector OwnerRightVector = CharacterOwner->GetActorRightVector();
    const FVector OwnerForwardVector = CharacterOwner->GetActorForwardVector();

	Right = OwnerLocation + (OwnerRightVector * TraceDistance) + (OwnerForwardVector * TraceAngle);
	Left = OwnerLocation + (OwnerRightVector * -TraceDistance) - (OwnerForwardVector * TraceAngle);
}

bool UParkourMovementComponent::IsValidImpactNormal(const FVector& ImpactNormal)
{
	return UKismetMathLibrary::InRange_FloatFloat(ImpactNormal.Z, -0.52f, 0.52f, false, false);
}

FVector UParkourMovementComponent::CalculateStickToWallVelocity()
{
        const FVector Location = CharacterOwner->GetActorLocation();
        const FVector Combined = Location + WallRunNormal;
        const float Length = Combined.Length();

        return WallRunNormal * Length;
}

void UParkourMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
       Super::GetLifetimeReplicatedProps(OutLifetimeProps);

       DOREPLIFETIME(UParkourMovementComponent, bIsWallRunning);
       DOREPLIFETIME(UParkourMovementComponent, bIsWallRunningLeft);
       DOREPLIFETIME(UParkourMovementComponent, bIsWallRunningRight);
       DOREPLIFETIME(UParkourMovementComponent, bIsWallSuppressed);
       DOREPLIFETIME(UParkourMovementComponent, InitialGravityScale);
       DOREPLIFETIME(UParkourMovementComponent, WallRunNormal);
}

void UParkourMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
    Super::UpdateFromCompressedFlags(Flags);

    bIsWallRunning = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
    bIsWallRunningLeft = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
    bIsWallRunningRight = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
    bIsWallSuppressed = (Flags & FSavedMove_Character::FLAG_Custom_3) != 0;
}

FNetworkPredictionData_Client* UParkourMovementComponent::GetPredictionData_Client() const
{
    check(PawnOwner != nullptr);

    if (!ClientPredictionData)
    {
        UParkourMovementComponent* MutableThis = const_cast<UParkourMovementComponent*>(this);
        MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Parkour(*this);
        MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
        MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
    }

    return ClientPredictionData;
}

void FSavedMove_Parkour::Clear()
{
    Super::Clear();
    bSavedIsWallRunning = false;
    bSavedIsWallRunningLeft = false;
    bSavedIsWallRunningRight = false;
    bSavedIsWallSuppressed = false;
}

uint8 FSavedMove_Parkour::GetCompressedFlags() const
{
    uint8 Result = Super::GetCompressedFlags();

    if (bSavedIsWallRunning)   Result |= FLAG_Custom_0;
    if (bSavedIsWallRunningLeft) Result |= FLAG_Custom_1;
    if (bSavedIsWallRunningRight) Result |= FLAG_Custom_2;
    if (bSavedIsWallSuppressed) Result |= FLAG_Custom_3;

    return Result;
}

void FSavedMove_Parkour::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
    Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

    const UParkourMovementComponent* MoveComp = Cast<UParkourMovementComponent>(Character->GetCharacterMovement());
    if (MoveComp)
    {
        bSavedIsWallRunning = MoveComp->bIsWallRunning;
        bSavedIsWallRunningLeft = MoveComp->bIsWallRunningLeft;
        bSavedIsWallRunningRight = MoveComp->bIsWallRunningRight;
        bSavedIsWallSuppressed = MoveComp->bIsWallSuppressed;
    }
}

void FSavedMove_Parkour::PrepMoveFor(ACharacter* Character)
{
    Super::PrepMoveFor(Character);

    UParkourMovementComponent* MoveComp = Cast<UParkourMovementComponent>(Character->GetCharacterMovement());
    if (MoveComp)
    {
        MoveComp->bIsWallRunning = bSavedIsWallRunning;
        MoveComp->bIsWallRunningLeft = bSavedIsWallRunningLeft;
        MoveComp->bIsWallRunningRight = bSavedIsWallRunningRight;
        MoveComp->bIsWallSuppressed = bSavedIsWallSuppressed;
    }
}

FNetworkPredictionData_Client_Parkour::FNetworkPredictionData_Client_Parkour(const UCharacterMovementComponent& ClientMovement)
    : Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Parkour::AllocateNewMove()
{
    return FSavedMovePtr(new FSavedMove_Parkour());
}
