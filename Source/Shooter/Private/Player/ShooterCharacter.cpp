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


AShooterCharacter::AShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	bUseControllerRotationYaw = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = true;

	// Initialize Pawn Capsule Collider
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(60.0f);

	// Initialize Third Person Mesh
	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;
	GetMesh()->bReceivesDecals = false;
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
	// Initialize FP Root
	FPRootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("FP_Root"));
	FPRootSceneComponent->SetupAttachment(RootComponent);

	FPMeshRootSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Mesh_Root"));
	FPMeshRootSpringArmComponent->SetupAttachment(FPRootSceneComponent);
	FPMeshRootSpringArmComponent->bUsePawnControlRotation = true;

	OffsetRootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Offset_Root"));
	OffsetRootSceneComponent->SetupAttachment(FPMeshRootSpringArmComponent);

	// Initialize First Person Mesh
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
	
	// Initialize Variables
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
	}
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnWeapon();
}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update Aim Offsets on the server
	if (HasAuthority())
	{
		FRotator AimOffsets = GetBaseAimRotation();
		AimOffsets.Normalize();
		
		AimOffsetPitch = FMath::Clamp(AimOffsets.Pitch, -90.0f, 90.0f);
		AimOffsetYaw   = FMath::Clamp(AimOffsets.Yaw, -90.0f, 90.0f);
	}
}

void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterCharacter, WeaponActor);
	DOREPLIFETIME(AShooterCharacter, AimOffsetYaw);
	DOREPLIFETIME(AShooterCharacter, AimOffsetPitch);
}

void AShooterCharacter::HandleMoveInput(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D MoveInputValue = Value.Get<FVector2D>();
	if (!MoveInputValue.IsNearlyZero())
	{
		AddMovementInput(GetActorForwardVector(), MoveInputValue.Y);
		AddMovementInput(GetActorRightVector(), MoveInputValue.X);
	}
}

void AShooterCharacter::HandleLookInput(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	FVector2D LookInputValue = Value.Get<FVector2D>();
	LookInputValue = LookInputValue * LookSensitivity;
	if (!LookInputValue.IsNearlyZero())
	{
		AddControllerYawInput(LookInputValue.X);
		AddControllerPitchInput(LookInputValue.Y);

		OnLookInputChanged.Broadcast(LookInputValue);
	}
}

void AShooterCharacter::HandleBeginFireInput(const FInputActionValue& Value)
{
	BeginFire();
}

void AShooterCharacter::HandleEndFireInput(const FInputActionValue& Value)
{
	EndFire();
}

void AShooterCharacter::BeginFire()
{
	if (WeaponActor)
	{
		WeaponActor->BeginFire();
	}
}

void AShooterCharacter::EndFire()
{
	if (WeaponActor)
	{
		WeaponActor->EndFire();
	}
}

void AShooterCharacter::SpawnWeapon()
{
	if (!IsValid(WeaponClass))
	{
		LOG_ERROR(TEXT("Spawn failed, WeaponClass is invalid!"));
		return;
	}
	
	if (!HasAuthority())
	{
		return;
	}

	// Initialize Spawn Parameters
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	WeaponActor = GetWorld()->SpawnActor<AWeaponBase>(WeaponClass, SpawnParameters);
	if (!IsValid(WeaponActor))
	{
		LOG_ERROR(TEXT("Spawn failed, WeaponActor is invalid!"));
		return;
	}
	
	OnRep_WeaponActor();
}

void AShooterCharacter::AttachWeaponToMesh()
{
	if (!WeaponActor)
	{
		return;
	}

	// Attach and set offset for FP weapon mesh
	WeaponActor->GetFPWeaponMeshComponent()->AttachToComponent(
		FPMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
	WeaponActor->GetFPWeaponMeshComponent()->SetRelativeLocation(WeaponActor->GetWeaponOffset());

	// Attach and set offset for TP weapon mesh
	WeaponActor->GetTPWeaponMeshComponent()->AttachToComponent(
		GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);
	WeaponActor->GetTPWeaponMeshComponent()->SetRelativeLocation(WeaponActor->GetWeaponOffset());
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