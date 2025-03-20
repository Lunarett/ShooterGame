#include "Weapon/WeaponBase.h"
#include "NiagaraFunctionLibrary.h"
#include "Debug/LoggerMacros.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/ShooterCharacter.h"
#include "Weapon/WeaponFireModeBase.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bNetUseOwnerRelevancy = true;

	// Initialize Root
	WeaponSceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = WeaponSceneRootComponent;

	// Initialize First Person Weapon Mesh
	FPWeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPWeaponMesh"));
	FPWeaponMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	FPWeaponMeshComponent->SetReceivesDecals(false);
	FPWeaponMeshComponent->SetCastShadow(false);
	FPWeaponMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	FPWeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FPWeaponMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	FPWeaponMeshComponent->SetOnlyOwnerSee(true);
	FPWeaponMeshComponent->SetupAttachment(WeaponSceneRootComponent);

	// Initialize Third Person Weapon Mesh
	TPWeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TPWeaponMesh"));
	TPWeaponMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	TPWeaponMeshComponent->SetReceivesDecals(false);
	TPWeaponMeshComponent->SetCastShadow(true);
	TPWeaponMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	TPWeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TPWeaponMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	TPWeaponMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	TPWeaponMeshComponent->SetOwnerNoSee(true);
	TPWeaponMeshComponent->SetupAttachment(WeaponSceneRootComponent);

	// Initialize Variables
	FireRate = 0.1f;
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OwnerShooterCharacter = Cast<AShooterCharacter>(GetOwner());
		if (!OwnerShooterCharacter)
		{
			LOG_ERROR_SCREEN("Failed to cast Owner to ShooterCharacter");
		}
	}

	if (FireModeClass)
	{
		FireModeBehavior = NewObject<UWeaponFireModeBase>(this, FireModeClass);
	}
	else
	{
		LOG_ERROR_SCREEN("FireModeClass is invalid")
	}
}

void AWeaponBase::Destroyed()
{
	Super::Destroyed();
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeaponBase, OwnerShooterCharacter);
}

void AWeaponBase::BeginFire()
{
	if (!HasAuthority())
	{
		ServerBeginFire();
		return;
	}

	if (FireModeBehavior && GetWorld())
	{
		FTimerDelegate FireDelegate;
		FireDelegate.BindUObject(this, &AWeaponBase::HandleWeaponFire);
		FireModeBehavior->InitializeFire(GetWorld(), FireTimerHandle, FireDelegate, FireRate);
	}
	else
	{
		LOG_ERROR_SCREEN("Fire mode is invalid")
	}
}

void AWeaponBase::EndFire()
{
	if (!HasAuthority())
	{
		ServerEndFire();
		return;
	}
	
	if (FireModeBehavior && GetWorld())
	{
		FireModeBehavior->StopFire(GetWorld(), FireTimerHandle);
	}
}

void AWeaponBase::HandleWeaponFire()
{
	if (!CanFire())
	{
		return;
	}

	OnWeaponFiredDelegate.Broadcast();

	if (FireCameraShake)
	{
		if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0); IsValid(CameraManager))
		{
			CameraManager->StartCameraShake(FireCameraShake);
		}
	}
	
	MulticastPlayWeaponFireAnimation();
	MulticastPlayMuzzleEffect();
	FireWeapon();
}

void AWeaponBase::ServerBeginFire_Implementation()
{
	BeginFire();
}

bool AWeaponBase::ServerBeginFire_Validate()
{
	return true;
}

void AWeaponBase::ServerEndFire_Implementation()
{
	EndFire();
}

bool AWeaponBase::ServerEndFire_Validate()
{
	return true;
}

void AWeaponBase::MulticastPlayWeaponFireAnimation_Implementation()
{
	PlayWeaponFireAnimation();
}

void AWeaponBase::MulticastPlayMuzzleEffect_Implementation()
{
	PlayMuzzleEffect();
}

void AWeaponBase::PlayWeaponFireAnimation()
{
	// Check if the owning character is valid
	if (!IsValid(OwnerShooterCharacter))
	{
		LOG_ERROR_SCREEN("OwnerShooterCharacter is invalid! Cannot play weapon fire animation.");
		return;
	}

	// Determine whether this character is locally controlled
	const bool bIsLocallyControlled = OwnerShooterCharacter->IsLocallyControlled();

	// Play First-Person Animation if locally controlled
	if (bIsLocallyControlled && WeaponFireAnimation.FPAnimationMontage)
	{
		USkeletalMeshComponent* FPMesh = OwnerShooterCharacter->GetFPMeshComponent();
		if (FPMesh && FPMesh->GetAnimInstance())
		{
			FPMesh->GetAnimInstance()->Montage_Play(WeaponFireAnimation.FPAnimationMontage);
		}
		else
		{
			LOG_WARN_SCREEN("First-Person Mesh or AnimInstance is null! Cannot play FP animation.");
		}
	}

	// Play Third-Person Animation for non-local players or on remote clients
	if (!bIsLocallyControlled && WeaponFireAnimation.TPAnimationMontage)
	{
		USkeletalMeshComponent* TPMesh = OwnerShooterCharacter->GetTPMeshComponent();
		if (TPMesh && TPMesh->GetAnimInstance())
		{
			TPMesh->GetAnimInstance()->Montage_Play(WeaponFireAnimation.TPAnimationMontage);
		}
		else
		{
			LOG_WARN_SCREEN("Third-Person Mesh or AnimInstance is null! Cannot play TP animation.");
		}
	}
}

void AWeaponBase::PlayMuzzleEffect()
{
	if (!MuzzleFlashEffect)
	{
		LOG_WARN_SCREEN("MuzzleFlashEffect is invalid! Cannot play muzzle effect.");
		return;
	}

	// Get the appropriate weapon mesh
	USkeletalMeshComponent* WeaponMesh = (OwnerShooterCharacter && OwnerShooterCharacter->IsLocallyControlled())
		? FPWeaponMeshComponent
		: TPWeaponMeshComponent;

	if (!WeaponMesh)
	{
		LOG_ERROR_SCREEN("WeaponMesh is invalid! Cannot spawn attached muzzle effect.");
		return;
	}

	// Spawn the Niagara system attached to the weapon mesh
	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		MuzzleFlashEffect,                  // The Niagara System to spawn
		WeaponMesh,                         // Attach to the weapon mesh
		WeaponMuzzleSocketName,             // Attach to this socket name
		FVector::ZeroVector,                // Location offset (relative to socket)
		FRotator::ZeroRotator,              // Rotation offset (relative to socket)
		EAttachLocation::SnapToTarget,      // Snap to the socket's transform
		true                                // Auto-destroy when effect finishes
	);
}

bool AWeaponBase::CanFire() const
{
	return true;
}

bool AWeaponBase::CanReload() const
{
	return true;
}

USkeletalMeshComponent* AWeaponBase::GetFPWeaponMeshComponent() const
{
	return FPWeaponMeshComponent;
}

USkeletalMeshComponent* AWeaponBase::GetTPWeaponMeshComponent() const
{
	return TPWeaponMeshComponent;
}
