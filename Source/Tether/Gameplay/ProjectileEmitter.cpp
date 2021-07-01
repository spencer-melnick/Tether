// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "ProjectileEmitter.h"

// Sets default values
AProjectileEmitter::AProjectileEmitter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	ProjectileEmitterComponent = CreateDefaultSubobject<UProjectileEmitterComponent>(TEXT("Projectile Emitter Component"));
	ProjectileEmitterComponent->ProjectileType = ProjectileClass;
	SetRootComponent(ProjectileEmitterComponent);

	ProjectileEmitterComponent->ProjectileLifetime = ProjectileLifetime;
	ProjectileEmitterComponent->ProjectileVelocity = Velocity;
	
	TelegraphComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("Telegraph Decal"));
	TelegraphComponent->SetupAttachment(RootComponent);
	TelegraphComponent->SetWorldScale3D(FVector(0.5));

	TelegraphComponent->SetDecalMaterial(TelegraphDecal);
	TelegraphComponent->SetRelativeRotation(FRotator(90, 90, 0));
	TelegraphComponent->SetVisibility(false);
	SetTelegraphSize();
}

void AProjectileEmitter::FireProjectile()
{
	if(bTelegraph)
	{		
		if(UWorld* World = GetWorld())
		{
			DisplayTelegraph();
			
			World->GetTimerManager().SetTimer(WarningHandle, FTimerDelegate::CreateWeakLambda(this, [this]
				{
					ProjectileEmitterComponent->FireProjectile();
					HideTelegraph();
				}), WarningTime, false);
		}
	}
	else
	{
		ProjectileEmitterComponent->FireProjectile();
	}
}

void AProjectileEmitter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ProjectileEmitterComponent->ProjectileType = ProjectileClass;
	ProjectileEmitterComponent->ProjectileVelocity = Velocity;
	ProjectileLifetime = (Velocity <= 0 || Distance <= 0) ? 0.0f : Distance / Velocity;
	ProjectileEmitterComponent->ProjectileLifetime = ProjectileLifetime;
	TelegraphComponent->SetMaterial(0, TelegraphDecal);
	SetTelegraphSize();
}


// Called when the game starts or when spawned
void AProjectileEmitter::BeginPlay()
{
	Super::BeginPlay();
	// ProjectileEmitterComponent->ProjectileLifetime = ProjectileLifetime;
}

void AProjectileEmitter::DisplayTelegraph()
{
	if(bTelegraph && TelegraphComponent)
	{
		TelegraphComponent->SetVisibility(true);
	}
}

void AProjectileEmitter::HideTelegraph()
{
	if(TelegraphComponent)
	{
		TelegraphComponent->SetVisibility(false);
	}
}

void AProjectileEmitter::SetTelegraphSize()
{
	const ASimpleProjectile* ActorCDO = ProjectileClass ? ProjectileClass->GetDefaultObject<ASimpleProjectile>() : nullptr;
	if (ActorCDO && TelegraphComponent)
	{
		const float Size = ActorCDO->ProjectileRadius * 2;
		TelegraphComponent->DecalSize = FVector(Size, Distance, Size);
		TelegraphComponent->SetRelativeLocation(FVector(Distance / 2.f, 0, 0));
	}
}

// Called every frame
void AProjectileEmitter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}