// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "SimpleProjectile.h"

#include "DrawDebugHelpers.h"
#include "Tether/Tether.h"
#include "Tether/Character/TetherCharacter.h"

ASimpleProjectile::ASimpleProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	// SetTickGroup(ETickingGroup::TG_DuringPhysics);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}


void ASimpleProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector NewLocation = GetActorLocation() + Velocity * DeltaSeconds;

	FHitResult HitResult;
	if (Sweep(Velocity * DeltaSeconds, HitResult))
	{
		UE_LOG(LogTetherGame, Verbose, TEXT("Projectile was going to hit something."))
		BouncePlayer(HitResult);
	}
	
	SetActorLocation(NewLocation, false);
}


void ASimpleProjectile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}


void ASimpleProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (!CollisionComponent)
	{
		if (UActorComponent* Component = GetComponentByClass(UShapeComponent::StaticClass()))
		{
			CollisionComponent = Cast<UShapeComponent>(Component);
		}
		else
		{
			UE_LOG(LogTetherGame, Warning, TEXT("Projectile does not have a valid collision shape."));
		}
	}
}

void ASimpleProjectile::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	BouncePlayer(Hit);
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
}


void ASimpleProjectile::BouncePlayer(const FHitResult& HitResult)
{
	if (ATetherCharacter* HitCharacter = Cast<ATetherCharacter>(HitResult.Actor))
	{
		if (!bDebounce)
		{
			OnProjectileBounce().Broadcast();
			// HitCharacter->Deflect(HitResult.ImpactNormal, LaunchVelocity, Velocity, 1.0, Elasticity, DeflectTime, true);

			const FVector RelativeVelocity = -HitResult.ImpactNormal * FVector::DotProduct(-HitResult.ImpactNormal, Velocity) * Elasticity;
			const FVector BounceVelocity = (-HitResult.ImpactNormal * LaunchVelocity + RelativeVelocity);
			HitCharacter->DeflectSimple(BounceVelocity, DeflectTime);
			if (UWorld* World = GetWorld())
			{
				bDebounce = true;
				World->GetTimerManager().SetTimer(DebounceTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
				{
					bDebounce = false;
				}), DebounceTime, false);
			}
		}
	}
}


bool ASimpleProjectile::Sweep(const FVector& Distance, FHitResult& OutHit) const
{
	if (!CollisionComponent)
	{
		return false;
	}
	if (Distance.IsZero())
	{
		return false;
	}
	if(UWorld* World = GetWorld())
	{
		const FVector Start = CollisionComponent->GetComponentLocation();
		const FVector End = Start + Distance;
		
		FCollisionQueryParams QueryParams;
		FCollisionResponseParams ResponseParams;

		QueryParams.bIgnoreTouches = false;

		CollisionComponent->InitSweepCollisionParams(QueryParams, ResponseParams);

		ResponseParams.CollisionResponse.SetResponse(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
		return World->SweepSingleByChannel(OutHit, Start, End, CollisionComponent->GetComponentQuat(), CollisionComponent->GetCollisionObjectType(),
			CollisionComponent->GetCollisionShape(), QueryParams, ResponseParams);
		
	}
	return false;
}