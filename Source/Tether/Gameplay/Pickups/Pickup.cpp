// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "Pickup.h"

#include "Tether/GameMode/TetherPrimaryGameState.h"


APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.SetTickFunctionEnable(true);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Sphere"));
	PickupSphere->SetCollisionProfileName(TEXT("OverlapPawnOnly"));
	RootComponent = PickupSphere;
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));
	Mesh->SetupAttachment(RootComponent);
}


void APickup::BeginPlay()
{
	GetWorld()->GetTimerManager().SetTimer(WarmupTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		// Effectively prevents the object from being picked up too soond
		bIsPickedUp = false;
	}), WarmupTime, false);
	Super::BeginPlay();
}


void APickup::Tick(const float DeltaTime)
{
	Mesh->AddRelativeRotation(FRotator(0.0f, 360.0f, 0.0f) * DeltaTime);
	if (bIsPickedUp)
	{
		return;
	}
	if (ATetherPrimaryGameState* GameState = Cast<ATetherPrimaryGameState>(GetWorld()->GetGameState()))
	{
		const FVector ClosestPlayerLocation = GameState->GetClosestCharacterPawn(GetActorLocation())->GetActorLocation();
		const float DistanceToPlayerSq = FVector::DistSquared(ClosestPlayerLocation, GetActorLocation());
		if (DistanceToPlayerSq < MaxAttractRadius * MaxAttractRadius)
		{
			const float AttractVelocity = FMath::Lerp(MaxAttractVelocity, MinAttractVelocity, DistanceToPlayerSq / (MaxAttractRadius * MaxAttractRadius));
			const FVector MovementDelta = FMath::VInterpConstantTo(GetActorLocation(), ClosestPlayerLocation, DeltaTime, AttractVelocity);

			Velocity = (GetActorLocation() - GetActorLocation()) * DeltaTime;

			SetActorLocation(MovementDelta, false);
		}
		else
		{
			Velocity += GetWorld()->GetGravityZ() * FVector::UpVector * DeltaTime;
			
			FHitResult SweepHit;
			const FCollisionObjectQueryParams CollisionObjectQueryParams;
			GetWorld()->SweepSingleByObjectType(SweepHit, GetActorLocation(), GetActorLocation() + Velocity * DeltaTime, FQuat::Identity, CollisionObjectQueryParams, PickupSphere->GetCollisionShape());
			
			if (SweepHit.bBlockingHit)
			{
				SetActorLocation(SweepHit.Location, false);
				Velocity *= FVector::OneVector - SweepHit.Normal;
			}
			else
			{
				SetActorLocation(GetActorLocation() + Velocity * DeltaTime, false);
			}
		}
	}
}

void APickup::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (bIsPickedUp)
	{
		return;
	}
	if (OtherActor->IsA(ATetherCharacter::StaticClass()))
	{
		if (ATetherPrimaryGameState* GameState = Cast<ATetherPrimaryGameState>(GetWorld()->GetGameState()))
		{
			GameState->AddScore(PointValue);
		}

		bIsPickedUp = true;
		OnPickedUp();
	}
}


void APickup::Suspend()
{
	PrimaryActorTick.SetTickFunctionEnable(false);
}

void APickup::Unsuspend()
{
	PrimaryActorTick.SetTickFunctionEnable(true);
}

void APickup::CacheInitialState()
{
}

void APickup::Reload()
{
	Destroy();
}

