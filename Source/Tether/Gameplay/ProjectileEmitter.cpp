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
	
	FloorMarkerComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("Telegraph Decal"));
	FloorMarkerComponent->SetupAttachment(RootComponent);
	FloorMarkerComponent->SetWorldScale3D(FVector(0.5));

	FloorMarkerComponent->SetDecalMaterial(FloorMarkerDecalMaterial);
	FloorMarkerComponent->SetRelativeRotation(FRotator(90, 90, 0));
	FloorMarkerComponent->SetVisibility(false);
	RecalculateFloorMarkerSize();
}


void AProjectileEmitter::FireProjectile()
{
	if(bTelegraph)
	{
		if(QueuedProjectiles < MaxQueuedProjectiles)
		{
			if(UWorld* World = GetWorld())
			{
				DisplayFloorMarker();
				
				World->GetTimerManager().SetTimer(WarningHandle, FTimerDelegate::CreateWeakLambda(this, [this]
					{
						HideFloorMarker();
					}), WarningTime * 1.2f, false);

				FTimerHandle FireHandle;
				QueuedProjectiles++;
				World->GetTimerManager().SetTimer(FireHandle, FTimerDelegate::CreateWeakLambda(this, [this]
					{
						ProjectileEmitterComponent->FireProjectile();
						QueuedProjectiles--;
					}), WarningTime, false);
			}
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
	ProjectileLifetime = Velocity <= 0 || Distance <= 0 ? 0.0f : Distance / Velocity;
	ProjectileEmitterComponent->ProjectileLifetime = ProjectileLifetime;
	if (FloorMarkerComponent && FloorMarkerDecalMaterial)
	{
		FloorMarkerComponent->SetMaterial(0, FloorMarkerDecalMaterial);
	}
	RecalculateFloorMarkerSize();
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

	ArrowMaterialDynamic = UMaterialInstanceDynamic::Create(ArrowMaterial, this);
	FloorMarkerMaterialDynamic = UMaterialInstanceDynamic::Create(FloorMarkerDecalMaterial, this);

	SetArrowMaterial(ArrowMaterialDynamic);
	SetFloorMarkerMaterial(FloorMarkerMaterialDynamic);
}


void AProjectileEmitter::DisplayFloorMarker()
{
	if(bTelegraph && FloorMarkerComponent)
	{
		FloorMarkerComponent->SetVisibility(true);
	}
	for (USkeletalMeshComponent* Component: ArrowComponents)
	{
		Component->SetVisibility(true);
	}
}


void AProjectileEmitter::HideFloorMarker()
{
	if(FloorMarkerComponent)
	{
		FloorMarkerComponent->SetVisibility(false);
	}
	for (USkeletalMeshComponent* Component: ArrowComponents)
	{
		Component->SetVisibility(false);
	}
}


void AProjectileEmitter::RecalculateFloorMarkerSize()
{
	const ASimpleProjectile* ActorCDO = ProjectileClass ? ProjectileClass->GetDefaultObject<ASimpleProjectile>() : nullptr;
	if (ActorCDO && FloorMarkerComponent && ArrowMesh)
	{
		const float Size = ProjectileEmitterComponent->ProjectileRadius * 2;
		FloorMarkerComponent->DecalSize = FVector(Size, Distance + Size, Size);
		FloorMarkerComponent->SetRelativeLocation(FVector((Distance / 2.f), 0, 0));

		for (USkeletalMeshComponent* Component: ArrowComponents)
		{
			if(Component)
			{
				Component->DestroyComponent();
			}
		}
		ArrowComponents.Empty();

		const FVector ArrowScale = FVector(Size / 100, Size / 100, Size / 1000);
		const int ArrowCount = FMath::Floor(Distance / Size / 2) + 2;

		for (int i = 0; i < ArrowCount; i++)
		{
			USkeletalMeshComponent* Arrow = NewObject<USkeletalMeshComponent>(RootComponent);
			Arrow->RegisterComponent();
			ArrowComponents.Add(Arrow);
			
			Arrow->SetSkeletalMesh(ArrowMesh);
			Arrow->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			Arrow->AddLocalOffset(FVector((Size * 2 * i) - Size, 0.f, 0.f));
			Arrow->SetWorldScale3D(ArrowScale);
			Arrow->SetRelativeRotation(FRotator(0,180,0));
			Arrow->SetVisibility(false);

			// Arrow->SetMaterial(0, ArrowMaterialDynamic);
		}
	}
}


void AProjectileEmitter::SetArrowMaterial(UMaterialInstanceDynamic* Material)
{
	if (Material)
	{
		for (USkeletalMeshComponent* Arrow : ArrowComponents)
		{
			Arrow->SetMaterial(0, Material);
		}
	}
}


void AProjectileEmitter::SetFloorMarkerMaterial(UMaterialInstanceDynamic* Material)
{
	FloorMarkerComponent->SetMaterial(0, Material);
}


// Called every frame
void AProjectileEmitter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float AlphaTime = 0.5f;
	if (UWorld* World = GetWorld())
	{
		AlphaTime = World->GetTimerManager().GetTimerRemaining(WarningHandle) / WarningTime;
	}

	// Prevent the value from going down when the timer resets
	if (AlphaCurve)
	{
		if (AlphaTime < 0.5f)
		{
			AlphaFallback = AlphaCurve->GetFloatValue(AlphaTime);
		}
		else
		{
			AlphaFallback = FMath::Max(AlphaFallback, AlphaCurve->GetFloatValue(AlphaTime));
		}
		ArrowMaterialDynamic->SetScalarParameterValue(TEXT("Opacity"), AlphaFallback);
		FloorMarkerMaterialDynamic->SetScalarParameterValue(TEXT("Opacity"), AlphaFallback);
	}
	// DisplayWarning(AlphaTime);
}