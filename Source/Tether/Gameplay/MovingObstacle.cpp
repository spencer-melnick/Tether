// Copyright (c) 2021 Spencer Melnick

#include "MovingObstacle.h"

#include "Components/CapsuleComponent.h"
#include "Tether/Character/TetherCharacter.h"
#include "Tether/Gamemode/TetherPrimaryGameMode.h"
#include "Tether/GameMode/TetherPrimaryGameState.h"

AMovingObstacle::AMovingObstacle()
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AMovingObstacle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	const UWorld* World = GetWorld();
	const ATetherPrimaryGameState* GameState = World ? World->GetGameState<ATetherPrimaryGameState>() : nullptr;
	if (GameState)
	{		
		const FVector DeltaLocation = GetActorForwardVector() * GameState->GetBaseObstacleSpeed() * SpeedMultiplier * DeltaSeconds;
		const FVector NewLocation = GetActorLocation() + DeltaLocation;

		if (const UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(RootComponent))
		{
			TArray<FHitResult> OutHits;

			FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
			QueryParams.bFindInitialOverlaps = true;

			FCollisionResponseParams ResponseParams = FCollisionResponseParams::DefaultResponseParam;
			ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
			ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Overlap);

			World->SweepMultiByChannel(OutHits,
				PrimitiveComponent->GetComponentLocation(), PrimitiveComponent->GetComponentLocation() + DeltaLocation, PrimitiveComponent->GetComponentQuat(),
				ECC_Pawn, PrimitiveComponent->GetCollisionShape(), QueryParams, ResponseParams);

			for (const FHitResult& HitResult : OutHits)
			{
				ATetherCharacter* Character = Cast<ATetherCharacter>(HitResult.Actor.Get());
				const bool bHitPawn = Character && Cast<UCapsuleComponent>(HitResult.Component) == Character->GetCapsuleComponent();
				const bool bIsPawnBase = Character && ACharacter::GetMovementBaseActor(Character) == this;
				
				if (bHitPawn && !bIsPawnBase)
				{
					FHitResult ReversedHit = FHitResult::GetReversedHit(HitResult);

					// If this result was not initially penetrating, figure out how far it theoretically penetrated the pawn
					if (!HitResult.bStartPenetrating)
					{
						ReversedHit.bStartPenetrating = true;
						ReversedHit.PenetrationDepth = (1.f - HitResult.Time) * DeltaLocation.Size();
					}
					
					Character->HandlePenetration(ReversedHit);
				}
			}
		}
		
		SetActorLocation(NewLocation);
	}

	// Only try to cull obstacles outside the game area on the server (can't destroy them on the client anyways)
#if WITH_SERVER_CODE
	const ATetherPrimaryGameMode* GameMode = World ? World->GetAuthGameMode<ATetherPrimaryGameMode>() : nullptr;
	if (GetNetMode() < NM_Client && GameMode)
	{
		const AVolume* ObstacleVolume = GameMode->GetObstacleVolume();
		if (ObstacleVolume && !ObstacleVolume->IsOverlappingActor(this))
		{
			Destroy();
		}
	}
#endif
}

