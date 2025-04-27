#include "Player/ShooterCharacter.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Debug/LoggerMacros.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/WeaponBase.h"
#include "AIController.h"
#include "Inventory/InventoryComponent.h"
#include "Animation/AnimInstance.h"

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

	CameraRootSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera_Root"));
	CameraRootSpringArmComponent->SetupAttachment(FPRootSceneComponent);
	CameraRootSpringArmComponent->bUsePawnControlRotation = true;

	CameraSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Camera_SkeletalMesh"));
	CameraSkeletalMesh->SetupAttachment(CameraRootSpringArmComponent);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera_Component"));
	CameraComponent->SetupAttachment(CameraSkeletalMesh);

	GetCharacterMovement()->MaxWalkSpeed = 1200.0f;
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->JumpZVelocity = 720.0f;
	GetCharacterMovement()->AirControl = 2.0f;
	GetCharacterMovement()->AirControlBoostMultiplier = 4.0f;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	WeaponSocketName = TEXT("WeaponPoint");
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Triggered, this,
		                                   &AShooterCharacter::HandleMoveInput);
		EnhancedInputComponent->BindAction(LookInputAction, ETriggerEvent::Triggered, this,
		                                   &AShooterCharacter::HandleLookInput);
		EnhancedInputComponent->BindAction(JumpInputAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(FireInputAction, ETriggerEvent::Started, this,
		                                   &AShooterCharacter::HandleBeginFireInput);
		EnhancedInputComponent->BindAction(FireInputAction, ETriggerEvent::Completed, this,
		                                   &AShooterCharacter::HandleEndFireInput);
		EnhancedInputComponent->BindAction(ReloadInputAction, ETriggerEvent::Started, this,
		                                   &AShooterCharacter::HandleWeaponReloadInput);

		EnhancedInputComponent->BindAction(NextWeaponInputAction, ETriggerEvent::Started, this,
		                                   &AShooterCharacter::HandleNextWeaponInput);
		EnhancedInputComponent->BindAction(PreviousWeaponInputAction, ETriggerEvent::Started, this,
		                                   &AShooterCharacter::HandlePreviousWeaponInput);
	}
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
}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		FRotator AimOffsets = GetBaseAimRotation();
		AimOffsets.Normalize();
		AimOffsetPitch = FMath::Clamp(AimOffsets.Pitch, -90.0f, 90.0f);
		AimOffsetYaw = FMath::Clamp(AimOffsets.Yaw, -90.0f, 90.0f);
	}
}

void AShooterCharacter::HandleMoveInput(const FInputActionValue& Value)
{
	if (!Controller) return;
	const FVector2D MoveInputValue = Value.Get<FVector2D>();
	if (!MoveInputValue.IsNearlyZero())
	{
		AddMovementInput(GetActorForwardVector(), MoveInputValue.Y);
		AddMovementInput(GetActorRightVector(), MoveInputValue.X);
	}
}

void AShooterCharacter::HandleLookInput(const FInputActionValue& Value)
{
	if (!Controller) return;
	FVector2D LookInputValue = Value.Get<FVector2D>() * LookSensitivity;
	if (!LookInputValue.IsNearlyZero())
	{
		AddControllerYawInput(LookInputValue.X);
		AddControllerPitchInput(LookInputValue.Y);
		OnLookInputChanged.Broadcast(LookInputValue);
	}
}

void AShooterCharacter::HandleBeginFireInput(const FInputActionValue& Value) { BeginFire(); }

void AShooterCharacter::HandleEndFireInput(const FInputActionValue& Value) { EndFire(); }

void AShooterCharacter::HandleWeaponReloadInput(const FInputActionValue& Value) { ReloadWeapon(); }

void AShooterCharacter::HandleNextWeaponInput(const FInputActionValue& Value)
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->EquipNextWeapon();
	}
}

void AShooterCharacter::HandlePreviousWeaponInput(const FInputActionValue& Value)
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->EquipPreviousWeapon();
	}
}

void AShooterCharacter::OnWeaponEquipped(float EquipCooldownDuration)
{
	if (!IsValid(InventoryComponent) || !InventoryComponent->GetEquippedWeapon())
	{
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

void AShooterCharacter::AttachWeaponToMesh()
{
	if (!WeaponActor)
	{
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
	if (!EquipMontage) return;

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

bool AShooterCharacter::IsPawnAIControlled() const
{
	return Controller && Controller->IsA<AAIController>();
}

void AShooterCharacter::GetAimOffsetValues(float& OutYaw, float& OutPitch) const
{
	OutYaw = AimOffsetYaw;
	OutPitch = AimOffsetPitch;
}
