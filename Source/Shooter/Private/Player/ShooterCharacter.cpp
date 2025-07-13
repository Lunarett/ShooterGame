#include "Player/ShooterCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Debug/LoggerMacros.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Health/HealthComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/ParkourMovementComponent.h"
#include "Player/ShooterPlayerController.h"
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
	TPSpringArmComponent->TargetArmLength = 300.0f;
	TPSpringArmComponent->SocketOffset = FVector(0, 50, 50);
	TPSpringArmComponent->SetRelativeLocation(FVector(0, 0, 96));

	ParkourMovementComponent = CreateDefaultSubobject<UParkourMovementComponent>(TEXT("Parkour Movement"));

	GetCharacterMovement()->MaxWalkSpeed = 1200.0f;
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->JumpZVelocity = 720.0f;
	GetCharacterMovement()->AirControl = 2.0f;
	GetCharacterMovement()->AirControlBoostMultiplier = 4.0f;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));

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

	// Bind On Weapon Equipped Delegate & Equip first weapon
	if (InventoryComponent)
	{
		InventoryComponent->OnWeaponEquippedDelegate.AddDynamic(this, &AShooterCharacter::OnWeaponEquipped);
		InventoryComponent->EquipWeaponByIndex(0);
	}

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AShooterCharacter::HandleCharacterDeath);
	}

	SetPlayerViewMode(EPlayerViewMode::FirstPerson);
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

	SetPlayerViewMode(ViewMode);
}

void AShooterCharacter::Kill()
{
	UGameplayStatics::ApplyDamage(
		this,
		9999.0f,
		GetController(),
		nullptr,
		UDamageType::StaticClass()
	);
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
	
	ViewMode = IsPawnAIControlled() ? EPlayerViewMode::FirstPerson : NewViewMode;

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

	if (WeaponActor)
	{
		WeaponActor->SetViewMode(NewViewMode);
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

void AShooterCharacter::HandleCharacterDeath(AController* InstigatedBy, AActor* DamageCauser)
{
	// Remove and destroy weapon
	if (InventoryComponent && WeaponActor)
	{
		InventoryComponent->RemoveWeapon(WeaponActor);
		WeaponActor->Destroy();
		WeaponActor = nullptr;
	}

	// Disable movement and input
	GetCharacterMovement()->DisableMovement();
	DisableInput(nullptr);

	// Enable ragdoll
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();

	// Detach controller
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->UnPossess();
	}

	// Delay camera setup
	FTimerHandle CameraTimerHandle;
	GetWorldTimerManager().SetTimer(CameraTimerHandle, [this, PC]()
	{
		if (!PC) return;

		// Spawn a camera actor behind and above the character
		const FVector Offset = FVector(-300, 0, 150);
		const FVector InitialLocation = GetActorLocation() + Offset;
		const FRotator InitialRotation = (GetActorLocation() - InitialLocation).Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ACameraActor* TrackingCamera = GetWorld()->SpawnActor<ACameraActor>(
			InitialLocation, InitialRotation, SpawnParams);
		if (!TrackingCamera) return;

		PC->SetViewTargetWithBlend(TrackingCamera, 1.0f);

		// Attach a tick-like update to track the ragdoll
		FTimerHandle FollowTimerHandle;
		GetWorldTimerManager().SetTimer(FollowTimerHandle, [this, TrackingCamera]()
		{
			if (!IsValid(this) || !IsValid(TrackingCamera)) return;

			const FVector FocusPoint = GetMesh()->GetComponentLocation();
			const FVector CamLocation = FocusPoint + FVector(-300, 0, 150);
			const FRotator CamRotation = (FocusPoint - CamLocation).Rotation();

			TrackingCamera->SetActorLocation(CamLocation);
			TrackingCamera->SetActorRotation(CamRotation);
		}, 0.02f, true); // runs every frame (50 FPS)
	}, 0.3f, false);
}
