#include "ProjectileWeapon.h"
#include "Weapon/Projectile.h"
#include "Player/ShooterCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"

AProjectileWeapon::AProjectileWeapon()
{
	// You may set a default ProjectileClass here if desired:
	// static ConstructorHelpers::FClassFinder<AProjectile> ProjectileBPClass(TEXT("/Game/Blueprints/BP_Projectile"));
	// if (ProjectileBPClass.Class) ProjectileClass = ProjectileBPClass.Class;

	// Set a default bullet spread (e.g., 5 degrees).
	BulletSpread = 5.f;
}

void AProjectileWeapon::FireWeapon()
{
	// Ensure a projectile class is defined
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ProjectileClass not set in %s"), *GetName());
		return;
	}

	// Get our owning character
	AShooterCharacter* Shooter = Cast<AShooterCharacter>(GetOwner());
	if (!Shooter)
	{
		UE_LOG(LogTemp, Error, TEXT("Owner is not a ShooterCharacter in %s"), *GetName());
		return;
	}

	// Use the first-person weapon mesh to get the muzzle socket location.
	USkeletalMeshComponent* FPWeaponMesh = GetFPWeaponMeshComponent();
	if (!FPWeaponMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("FPWeaponMesh not found in %s"), *GetName());
		return;
	}

	// Get muzzle location from the socket (as defined in your base class)
	FVector MuzzleLocation = FPWeaponMesh->GetSocketLocation(WeaponMuzzleSocketName);

	// Get the player’s camera viewpoint
	FVector ViewLocation;
	FRotator ViewRotation;
	if (APlayerController* PC = Cast<APlayerController>(Shooter->GetController()))
	{
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		// Fallback: use the character’s eye location/rotation.
		ViewLocation = Shooter->GetActorLocation();
		ViewRotation = Shooter->GetActorRotation();
	}

	// Determine target point by doing a line trace from the camera (center of screen) out into the world.
	FVector TraceStart = ViewLocation;
	FVector TraceEnd = TraceStart + (ViewRotation.Vector() * 10000.f); // long distance
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(Shooter);

	FVector TargetPoint = TraceEnd;
	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		TargetPoint = HitResult.ImpactPoint;
	}

	// Compute the firing direction based on the muzzle location and the target point.
	FVector FireDirection = (TargetPoint - MuzzleLocation).GetSafeNormal();

	// Apply bullet spread by randomizing the fire direction.
	// Convert the bullet spread from degrees to radians and take half the value.
	float HalfConeRad = FMath::DegreesToRadians(BulletSpread * 0.5f);
	FVector SpreadFireDirection = FMath::VRandCone(FireDirection, HalfConeRad);

	// (Optional) Debug line to visualize the spread direction:
	// DrawDebugLine(GetWorld(), MuzzleLocation, MuzzleLocation + SpreadFireDirection * 1000.f, FColor::Red, false, 2.f);

	// Set up spawn parameters – ensure the actor is spawned on the server.
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = Shooter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the projectile; note that since this is called on the server (or via an RPC), it will replicate to clients.
	AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, MuzzleLocation, SpreadFireDirection.Rotation(), SpawnParams);
	if (Projectile)
	{
		// Let the projectile know its fire direction (with spread).
		Projectile->FireInDirection(SpreadFireDirection);
	}
}