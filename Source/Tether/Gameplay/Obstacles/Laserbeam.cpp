// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "Laserbeam.h"

#include "Components/ArrowComponent.h"

ALaserbeam::ALaserbeam()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	
	LaserSource = CreateDefaultSubobject<USceneComponent>(TEXT("Laser Source"));
	LaserSource->SetupAttachment(RootComponent);
	
	LaserMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Laser Mesh Component"));
	LaserMeshComponent->SetupAttachment(LaserSource);

	SmokeEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Smoke Effect"));
	SmokeEffect->SetupAttachment(RootComponent);
	SmokeEffect->SetUsingAbsoluteRotation(true);

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	ArrowComponent->SetupAttachment(LaserSource);
	ArrowComponent->SetVisibleFlag(true);
	ArrowComponent->SetHiddenInGame(true);
	ArrowComponent->ArrowLength = MaxLength;
	ArrowComponent->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
}

void ALaserbeam::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
		{
			Destroy();
		}), Lifetime, false);
	}
}


void ALaserbeam::Tick(float DeltaTime)
{
	AddActorWorldOffset(Velocity * DeltaTime);
	TraceLaser(GetActorLocation(), LaserMeshComponent->GetUpVector(), MaxLength, DeltaTime);

	ResizeLaserMesh();
}


void ALaserbeam::OnConstruction(const FTransform& Transform)
{
	FVector Max, Min;
	LaserMeshComponent->GetLocalBounds(Max, Min);
	LaserMeshLength = FMath::Abs(Min.Z - Max.Z);

	ArrowComponent->ArrowLength = MaxLength;

	ResizeLaserMesh();
}


void ALaserbeam::TraceLaser(const FVector Location, const FVector DirectionVector, const float MaxDistance, const float DeltaTime)
{
	if (UWorld* World = GetWorld())
	{
		FHitResult HitResult;

		if (World->LineTraceSingleByProfile(HitResult, Location, Location + DirectionVector.GetSafeNormal() * MaxDistance, CollisionProfileName.Name))
		{
			if (IsValid(SmokeEffect))
			{
				SmokeEffect->SetWorldLocation(HitResult.Location);
				SmokeEffect->Activate();
			}

			const FDamageEvent DamageEvent;
			HitResult.Actor->TakeDamage(LaserDamage * DeltaTime, DamageEvent, GetInstigatorController(), this);
			LaserLength = HitResult.Distance;
		}
		else
		{
			if (IsValid(SmokeEffect))
			{
				SmokeEffect->Deactivate();
			}
			
			LaserLength = MaxLength;
		}
	}
}

void ALaserbeam::ResizeLaserMesh()
{
	if (FMath::IsNearlyZero(LaserMeshLength))
	{
		LaserMeshLength = 100.0f;
	}
	
	const FVector CurrentScale = LaserMeshComponent->GetComponentScale();
	LaserMeshComponent->SetWorldScale3D(FVector(CurrentScale.X, CurrentScale.Y, LaserLength / LaserMeshLength));
	LaserMeshComponent->SetWorldLocation(GetActorLocation() + (LaserMeshComponent->GetUpVector() * LaserLength / 2.0f));
}
