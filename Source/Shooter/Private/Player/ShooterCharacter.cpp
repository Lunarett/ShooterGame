#include "Player/ShooterCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Debug/LoggerMacros.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/WeaponBase.h"

AShooterCharacter::AShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(60.0f);

	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;
	GetMesh()->bReceivesDecals = false;
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	FPRootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("FP_Root"));
	FPRootSceneComponent->SetupAttachment(RootComponent);

	FPMeshRootSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Mesh_Root"));
	FPMeshRootSpringArmComponent->SetupAttachment(FPRootSceneComponent);
	FPMeshRootSpringArmComponent->bUsePawnControlRotation = true;

	OffsetRootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Offset_Root"));
	OffsetRootSceneComponent->SetupAttachment(FPMeshRootSpringArmComponent);

	FPMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FPMesh->SetupAttachment(OffsetRootSceneComponent);
	FPMesh->SetRelativeLocation(FVector(0, 0, -96));
	FPMesh->SetOnlyOwnerSee(true);
	FPMesh->SetOwnerNoSee(false);
	FPMesh->SetCastShadow(false);
	FPMesh->SetReceivesDecals(false);
	FPMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	FPMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	FPMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FPMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	FPCameraRootSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera_Root"));
	FPCameraRootSpringArmComponent->SetupAttachment(FPRootSceneComponent);
	FPCameraRootSpringArmComponent->bUsePawnControlRotation = true;

	CameraSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Camera_SkeletalMesh"));
	CameraSkeletalMesh->SetupAttachment(FPCameraRootSpringArmComponent);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera_Component"));
	CameraComponent->SetupAttachment(CameraSkeletalMesh);

	TPSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("TP Spring Arm"));
	TPSpringArmComponent->SetupAttachment(GetMesh());
	TPSpringArmComponent->bUsePawnControlRotation = true;
	TPSpringArmComponent->bInheritYaw = true;
	TPSpringArmComponent->TargetArmLength = 400.0f;
	TPSpringArmComponent->SetRelativeLocation(FVector(0, 0, 96));

	GetCharacterMovement()->MaxWalkSpeed = 1200.0f;
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->JumpZVelocity = 720.0f;
	GetCharacterMovement()->AirControl = 2.0f;
	GetCharacterMovement()->AirControlBoostMultiplier = 4.0f;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	WeaponSocketName = TEXT("WeaponPoint");
}

void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShooterCharacter, WeaponActor);
	DOREPLIFETIME(AShooterCharacter, AimOffsetYaw);
	DOREPLIFETIME(AShooterCharacter, AimOffsetPitch);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetPlayerViewMode(ViewMode);

	// Bind On Weapon Equipped Delegate & Equip first weapon
	if (InventoryComponent)
	{
		InventoryComponent->OnWeaponEquippedDelegate.AddDynamic(this, &AShooterCharacter::OnWeaponEquipped);
		InventoryComponent->EquipWeaponByIndex(0);
	}
}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		// Update aim offsets for TP Mesh
		FRotator AimOffsets = GetBaseAimRotation();
		AimOffsets.Normalize();
		AimOffsetPitch = FMath::Clamp(AimOffsets.Pitch, -90.0f, 90.0f);
		AimOffsetYaw = FMath::Clamp(AimOffsets.Yaw, -90.0f, 90.0f);
	}
}

void AShooterCharacter::OnWeaponEquipped(float EquipCooldownDuration)
{
	if (!IsValid(InventoryComponent) || !InventoryComponent->GetEquippedWeapon())
	{
		// Either your InventoryComponent is invalid, or your weapon was not equipped!
		check(IsValid(InventoryComponent) && InventoryComponent->GetEquippedWeapon());
		return;
	}

	WeaponActor = InventoryComponent->GetEquippedWeapon();

	bCanInteractWithWeapon = false;

	// Attach meshes
	AttachWeaponToMesh();

	// Play equip animation
	PlayEquipMontage(false);

	GetWorld()->GetTimerManager().SetTimer(EquipTimerHandle, [this]()
	{
		bCanInteractWithWeapon = true;
	}, EquipCooldownDuration, false);
}

void AShooterCharacter::BeginFire()
{
	if (!IsValid(InventoryComponent) || !IsValid(WeaponActor))
	{
		return;
	}

	if (bCanInteractWithWeapon)
	{
		WeaponActor->BeginFire();
	}
	else
	{
		WeaponActor->EndFire();
	}
}

void AShooterCharacter::EndFire()
{
	if (WeaponActor)
	{
		WeaponActor->EndFire();
	}
}

void AShooterCharacter::ReloadWeapon()
{
	if (!IsValid(InventoryComponent))
	{
		return;
	}

	if (!bCanInteractWithWeapon || !WeaponActor)
	{
		return;
	}

	if (InventoryComponent->GetAmmo(WeaponActor->GetAmmoData().AmmoTypeTag))
	{
		WeaponActor->BeginReload();
	}
}

int32 AShooterCharacter::RequestAmmo_Implementation(FGameplayTag AmmoType, int32 RequestedAmount)
{
	if (!InventoryComponent)
	{
		return 0;
	}

	const int32 Available = InventoryComponent->GetAmmo(AmmoType);
	if (Available <= 0)
	{
		return 0;
	}

	const int32 Provided = FMath::Min(RequestedAmount, Available);
	InventoryComponent->ConsumeAmmo(AmmoType, Provided);

	return Provided;
}

void AShooterCharacter::SetPlayerViewMode(EPlayerViewMode NewViewMode)
{
	if (!CameraComponent)
	{
		LOG_ERROR("Cannot change view mode, CameraComponent is invalid");
		return;
	}
	
	ViewMode = NewViewMode;

	switch (ViewMode)
	{
	case EPlayerViewMode::FirstPerson:
		FPMesh->SetOnlyOwnerSee(true);
		FPMesh->SetOwnerNoSee(false);
		GetMesh()->SetOnlyOwnerSee(false);
		GetMesh()->SetOwnerNoSee(true);
		CameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		CameraComponent->AttachToComponent(CameraSkeletalMesh, FAttachmentTransformRules::KeepRelativeTransform);
		CameraComponent->SetRelativeLocation(FVector::ZeroVector);
		CameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
		bUseControllerRotationPitch = true;
		bUseControllerRotationYaw = true;
		bUseControllerRotationRoll = false;
		break;
	case EPlayerViewMode::ThirdPerson:
		FPMesh->SetOnlyOwnerSee(false);
		FPMesh->SetOwnerNoSee(true);
		GetMesh()->SetOnlyOwnerSee(true);
		GetMesh()->SetOwnerNoSee(false);
		CameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		CameraComponent->AttachToComponent(TPSpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform);
		CameraComponent->SetRelativeLocation(FVector::ZeroVector);
		CameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
		bUseControllerRotationPitch = false;
		bUseControllerRotationYaw = true;
		bUseControllerRotationRoll = false;
		break;
	}
}

void AShooterCharacter::AttachWeaponToMesh()
{
	if (!IsValid(WeaponActor))
	{
		LOG_ERROR("Failed to attach weapon to character, WeaponActor is invalid");
		return;
	}

	// Attach FP Weapon + Apply Offset
	WeaponActor->GetFPWeaponMeshComponent()->AttachToComponent(
		FPMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
	WeaponActor->GetFPWeaponMeshComponent()->SetRelativeLocation(WeaponActor->GetWeaponOffset());

	// Attach TP Weapon + Apply Offset
	WeaponActor->GetTPWeaponMeshComponent()->AttachToComponent(
		GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
	WeaponActor->GetTPWeaponMeshComponent()->SetRelativeLocation(WeaponActor->GetWeaponOffset());
}

void AShooterCharacter::PlayEquipMontage(const bool bReverse)
{
	if (!EquipMontage)
	{
		LOG_WARN("Equip animation will not play, EquipMontage is invalid");
		return;
	}

	const float PlayRate = bReverse ? -1.0f : 1.0f;
	const float StartAt = bReverse ? EquipMontage->GetPlayLength() : 0.0f;

	if (UAnimInstance* AnimInstance = FPMesh->GetAnimInstance())
	{
		AnimInstance->Montage_Play(EquipMontage, PlayRate, EMontagePlayReturnType::MontageLength, StartAt, true);
	}
}

void AShooterCharacter::OnRep_WeaponActor()
{
	AttachWeaponToMesh();
}