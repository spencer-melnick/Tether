// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "SimpleProjectile.h"

#include "DrawDebugHelpers.h"
#include "Tether/Character/TetherCharacter.h"

ASimpleProjectile::ASimpleProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Component"));
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RootComponent = CollisionComponent;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh Component"));
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComponent->SetupAttachment(CollisionComponent);
}


void ASimpleProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector NewLocation = GetActorLocation() + Velocity * DeltaSeconds;

	FHitResult HitResult;
	if (Sweep(Velocity * DeltaSeconds, HitResult))
	{
		// GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Collision"));
		BouncePlayer(HitResult);
	}
	SetActorLocation(NewLocation, false);
}


void ASimpleProjectile::BouncePlayer(const FHitResult& HitResult)
{
	if (ATetherCharacter* HitCharacter = Cast<ATetherCharacter>(HitResult.Actor))
	{
		if (!bDebounce)
		{
			HitCharacter->Deflect(-HitResult.ImpactNormal, LaunchVelocity, Velocity, 1.0, Elasticity, DeflectTime, true);

			if (UWorld* World = GetWorld())
			{
				#if WITH_EDITOR
				if (GEngine->GameViewport && GEngine->GameViewport->EngineShowFlags.GameplayDebug)
				{
					DrawDebugDirectionalArrow(World, HitResult.ImpactPoint, HitResult.ImpactPoint + HitResult.ImpactNormal * -100.0f, 25.0f, FColor::Red, false, 3.0f, 0, 5.0f);
				}
				#endif
				
				bDebounce = true;
				World->GetTimerManager().SetTimer(DebounceTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
				{
					bDebounce = false;
				}), DebounceTime, false);
			}
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Collided with a non-player entity"));
	}
}


bool ASimpleProjectile::Sweep(const FVector& Distance, FHitResult& OutHit) const
{
	if(UWorld* World = GetWorld())
	{
		const FVector Start = CollisionComponent->GetComponentLocation();
		const FVector End = Start + Distance;
		
		FCollisionQueryParams QueryParams;
		FCollisionResponseParams ResponseParams;

		CollisionComponent->InitSweepCollisionParams(QueryParams, ResponseParams);

		ResponseParams.CollisionResponse.SetResponse(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
		return World->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, CollisionComponent->GetCollisionObjectType(),
			CollisionComponent->GetCollisionShape(), QueryParams, ResponseParams);
		
	}
	return false;
}