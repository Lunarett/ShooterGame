#include "Weapon/Projectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Enable replication.
	bReplicates = true;
	SetReplicateMovement(true);

	// Create collision component.
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComponent->InitSphereRadius(5.0f);
	CollisionComponent->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComponent->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	SetRootComponent(CollisionComponent);
	
	// Create Niagara effect component.
	ProjectileEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileEffectComponent"));
	ProjectileEffectComponent->SetupAttachment(RootComponent);
	
	// Create projectile movement component.
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->InitialSpeed = 3000.f;
	ProjectileMovementComponent->MaxSpeed = 3000.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;

	// Set default lifetime.
	LifeTime = 5.0f;

	// Set a default damage value.
	DamageAmount = 25.f;
	DecalSize = FVector(10.0f, 10.0f, 10.0f);  
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		CollisionComponent->IgnoreActorWhenMoving(OwnerActor, true);
	}

	APawn* InstigatorPawn = GetInstigator();
	if (InstigatorPawn)
	{
		CollisionComponent->IgnoreActorWhenMoving(InstigatorPawn, true);
	}

	GetWorldTimerManager().SetTimer(
		LifetimeTimerHandle,
		this, 
		&AProjectile::OnLifetimeExpired, 
		LifeTime, 
		false
	);
}

void AProjectile::FireInDirection(const FVector& ShootDirection)
{
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Velocity = ShootDirection * ProjectileMovementComponent->InitialSpeed;
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only apply damage if we hit a valid actor that is not our owner.
	if (OtherActor && OtherActor != GetOwner())
	{
		// Apply point damage to the hit actor.
		UGameplayStatics::ApplyPointDamage(OtherActor, DamageAmount, ProjectileMovementComponent->Velocity.GetSafeNormal(), Hit, GetInstigatorController(), this, UDamageType::StaticClass());
	}

	// Spawn the impact effect.
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}

	if (ImpactDecalMaterial)
	{
		UGameplayStatics::SpawnDecalAtLocation(
			GetWorld(),
			ImpactDecalMaterial,
			DecalSize,
			Hit.ImpactPoint,
			Hit.ImpactNormal.Rotation()
		);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Red, FString::Printf(TEXT("%s"), *OtherActor->GetName()));
	}

	Destroy();
}

void AProjectile::OnLifetimeExpired()
{
	Destroy();
}