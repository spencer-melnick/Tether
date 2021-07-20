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
	if (TelegraphComponent && TelegraphDecal)
	{
		TelegraphComponent->SetMaterial(0, TelegraphDecal);
	}
	SetTelegraphSize();
}


// Called when the game starts or when spawned
void AProjectileEmitter::BeginPlay()
{
	Super::BeginPlay();

	for (int i = 0; i < ArrowComponents.Num(); i++)
	{
		if(i == 0)
		{
			ArrowComponents[i]->PlayAnimation(BackAnimation, true);
		}
		else if(i == ArrowComponents.Num() - 1)
		{
			ArrowComponents[i]->PlayAnimation(FrontAnimation, true);
		}
		else
		{
			ArrowComponents[i]->PlayAnimation(MiddleAnimation, true);
		}
	}
}

void AProjectileEmitter::DisplayTelegraph()
{
	if(bTelegraph && TelegraphComponent)
	{
		TelegraphComponent->SetVisibility(true);
	}
	for (USkeletalMeshComponent* Component: ArrowComponents)
	{
		Component->SetVisibility(true);
	}

	DisplayWarning();
}

void AProjectileEmitter::HideTelegraph()
{
	if(TelegraphComponent)
	{
		TelegraphComponent->SetVisibility(false);
	}
	for (USkeletalMeshComponent* Component: ArrowComponents)
	{
		Component->SetVisibility(false);
	}
}

void AProjectileEmitter::SetTelegraphSize()
{
	const ASimpleProjectile* ActorCDO = ProjectileClass ? ProjectileClass->GetDefaultObject<ASimpleProjectile>() : nullptr;
	if (ActorCDO && TelegraphComponent && ArrowMaterial && ArrowMesh)
	{
		const float Size = ActorCDO->ProjectileRadius * 2;
		TelegraphComponent->DecalSize = FVector(Size, Distance + Size, Size);
		TelegraphComponent->SetRelativeLocation(FVector((Distance / 2.f), 0, 0));

		for (USkeletalMeshComponent* Component: ArrowComponents)
		{
			if(Component)
			{
				Component->DestroyComponent();
			}
		}
		ArrowComponents.Empty();

		const FVector ArrowScale = FVector(Size / 100, Size / 100, Size / 10000);
		const int ArrowCount = FMath::Floor(Distance / Size / 2) + 2;

		for (int i = 0; i < ArrowCount; i++)
		{
			USkeletalMeshComponent* Arrow = NewObject<USkeletalMeshComponent>(RootComponent);
			Arrow->RegisterComponent();
			ArrowComponents.Add(Arrow);
			
			Arrow->SetSkeletalMesh(ArrowMesh);
			Arrow->SetMaterial(0, ArrowMaterial);
			Arrow->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			Arrow->AddLocalOffset(FVector((Size * 2 * i) - Size, 0.f, 0.f));
			Arrow->SetWorldScale3D(ArrowScale);
			Arrow->SetRelativeRotation(FRotator(0,180,0));
			Arrow->SetVisibility(false);
		}
	}
}

// Called every frame
void AProjectileEmitter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}