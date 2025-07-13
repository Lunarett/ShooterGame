#include "Weapon/WeaponBase.h"

#include "EnhancedInputComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Debug/LoggerMacros.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/ShooterCharacter.h"
#include "Weapon/AmmoProvider.h"
#include "Weapon/WeaponFireModeBase.h"
#include "Weapon/WeaponRecoilComponent.h"

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
	//FPWeaponMeshComponent->SetOnlyOwnerSee(true);
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
	//TPWeaponMeshComponent->SetOwnerNoSee(true);
	TPWeaponMeshComponent->SetupAttachment(WeaponSceneRootComponent);

	// Initialize Weapon Recoil Component
	RecoilComponent = CreateDefaultSubobject<UWeaponRecoilComponent>(TEXT("Weapon Recoil Component"));

	// Initialize Variables
	FireRate = 0.1f;
	ReloadDuration = 1.0f;
	CurrentClipAmmo = AmmoData.ClipSize;
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);

        DOREPLIFETIME(AWeaponBase, OwnerShooterCharacter);
        DOREPLIFETIME(AWeaponBase, CurrentClipAmmo);
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
			return;
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

void AWeaponBase::OnLookInput(const FInputActionValue& Value)
{
	if (RecoilComponent)
	{
		const FVector2D MouseDelta = Value.Get<FVector2D>();

		// Add mouse delta for recoil to reset to the correct position
		RecoilComponent->OnYawAdded(MouseDelta.X);
		RecoilComponent->OnPitchAdded(MouseDelta.Y);
	}
}

void AWeaponBase::SetViewMode(EPlayerViewMode ViewMode)
{
	switch (ViewMode)
	{
	case EPlayerViewMode::FirstPerson:
		FPWeaponMeshComponent->SetVisibility(true);
		FPWeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		TPWeaponMeshComponent->SetVisibility(false);
		TPWeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EPlayerViewMode::ThirdPerson:
		TPWeaponMeshComponent->SetVisibility(true);
		TPWeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		FPWeaponMeshComponent->SetVisibility(false);
		FPWeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
}


// Weapon Fire
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

	// Enable input and bind LookInputAction
	if (APlayerController* PC = Cast<APlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		EnableInput(PC);

		if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
		{
			if (LookInputAction)
			{
				EnhancedInput->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &AWeaponBase::OnLookInput);
			}
			else
			{
				LOG_WARN_SCREEN("LookInputAction is not assigned.");
			}
		}
		else
		{
			LOG_ERROR_SCREEN("InputComponent is not an EnhancedInputComponent.");
		}
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

	--CurrentClipAmmo;

	// Play Weapon Fire Camera Shake
	if (FireCameraShake)
	{
		if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0); IsValid(
			CameraManager))
		{
			CameraManager->StartCameraShake(FireCameraShake);
		}
	}

	// Trigger Weapon Fire Animation
	MulticastPlayWeaponFireAnimation();

	// Spawn Weapon Fire Muzzle Effect
	MulticastPlayMuzzleEffect();

	// Play Recoil
	if (RecoilComponent && !OwnerShooterCharacter->IsPawnAIControlled())
	{
		RecoilComponent->AddRecoil();
	}

	// Execute actual Fire Logic
	FireWeapon();

	OnWeaponFiredDelegate.Broadcast();

	// If we consumed our last bullet, call reload
	if (bAutoReload && CurrentClipAmmo <= 0)
	{
		BeginReload();
	}
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


// Weapon Reload
void AWeaponBase::BeginReload()
{
	if (!CanReload())
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerBeginReload();
		return;
	}

	bIsReloadingWeapon = true;

	MulticastPlayWeaponReloadAnimation();

	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AWeaponBase::EndReload, ReloadDuration, false);
}

void AWeaponBase::EndReload()
{
	if (!OwnerShooterCharacter)
	{
		return;
	}

	if (OwnerShooterCharacter && OwnerShooterCharacter->GetClass()->ImplementsInterface(UAmmoProvider::StaticClass()))
	{
		const int32 AmmoNeeded = AmmoData.ClipSize - CurrentClipAmmo;
		const int32 AmmoReceived = IAmmoProvider::Execute_RequestAmmo(OwnerShooterCharacter, AmmoData.AmmoTypeTag,
		                                                              AmmoNeeded);
		CurrentClipAmmo += AmmoReceived;
	}

	bIsReloadingWeapon = false;
}

void AWeaponBase::ServerBeginReload_Implementation()
{
	BeginReload();
}

bool AWeaponBase::ServerBeginReload_Validate()
{
	return true;
}


// Weapon VFX
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
		MuzzleFlashEffect, // The Niagara System to spawn
		WeaponMesh, // Attach to the weapon mesh
		WeaponMuzzleSocketName, // Attach to this socket name
		FVector::ZeroVector, // Location offset (relative to socket)
		FRotator::ZeroRotator, // Rotation offset (relative to socket)
		EAttachLocation::SnapToTarget, // Snap to the socket's transform
		true // Auto-destroy when effect finishes
	);
}

void AWeaponBase::MulticastPlayMuzzleEffect_Implementation()
{
	PlayMuzzleEffect();
}


void AWeaponBase::PlayAnimationMontage(FWeaponAnimationData AnimationData)
{
	// Check if the owning character is valid
	if (!IsValid(OwnerShooterCharacter))
	{
		LOG_ERROR_SCREEN("OwnerShooterCharacter is invalid! Cannot play weapon fire animation.");
		return;
	}

	// Get Player's View Mode
	EPlayerViewMode ViewMode = OwnerShooterCharacter->GetViewMode();

	// Play First-Person Animation if locally controlled
	if (ViewMode == EPlayerViewMode::FirstPerson && AnimationData.FPAnimationMontage)
	{
		const USkeletalMeshComponent* FPMesh = OwnerShooterCharacter->GetFPMeshComponent();
		if (FPMesh && FPMesh->GetAnimInstance())
		{
			FPMesh->GetAnimInstance()->Montage_Play(AnimationData.FPAnimationMontage);
		}
		else
		{
			LOG_WARN_SCREEN("First-Person Mesh or AnimInstance is null! Cannot play FP animation.");
		}
	}

	// Play Third-Person Animation
	if (ViewMode == EPlayerViewMode::ThirdPerson && AnimationData.TPAnimationMontage)
	{
		const USkeletalMeshComponent* TPMesh = OwnerShooterCharacter->GetTPMeshComponent();
		if (TPMesh && TPMesh->GetAnimInstance())
		{
			TPMesh->GetAnimInstance()->Montage_Play(AnimationData.TPAnimationMontage);
		}
		else
		{
			LOG_WARN_SCREEN("Third-Person Mesh or AnimInstance is null! Cannot play TP animation.");
		}
	}
}

void AWeaponBase::MulticastPlayWeaponFireAnimation_Implementation()
{
	PlayAnimationMontage(WeaponFireAnimation);
}

void AWeaponBase::MulticastPlayWeaponReloadAnimation_Implementation()
{
	PlayAnimationMontage(WeaponReloadAnimation);
}

// Helpers & Getters
bool AWeaponBase::CanFire() const
{
	return CurrentClipAmmo > 0 && !bIsReloadingWeapon;
}

bool AWeaponBase::CanReload() const
{
        return !bIsReloadingWeapon && !AmmoData.bInfiniteAmmo && CurrentClipAmmo < AmmoData.ClipSize;
}

void AWeaponBase::OnRep_CurrentClipAmmo()
{
        // Placeholder for HUD update or any other visual logic when ammo changes
}
