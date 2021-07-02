// Copyright (c) 2021 Spencer Melnick

#include "MovingObstacle.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerState.h"
#include "Tether/Character/TetherCharacter.h"
#include "Tether/Gamemode/TetherPrimaryGameMode.h"
#include "Tether/GameMode/TetherPrimaryGameState.h"


namespace MovingObstacleCVars
{
	static float MinimumAdjustment = 50.f;
	static FAutoConsoleVariableRef CVarMinimumAdjustment(
		TEXT("MovingObstacle.MinimumAdjustment"), MinimumAdjustment,
		TEXT("The minimum distance between the server replicated positon and the client estimated position ")
		TEXT("after which the obstacle will teleport."),
		ECVF_Default);

	static float LookaheadFactor = 2.f;
	static FAutoConsoleVariable CVarLookaheadFactor(
		TEXT("MovingObstacle.LookaheadFactor"), LookaheadFactor,
		TEXT("How many (RTTs / 2) we should look ahead for obstacle movement prediction"),
		ECVF_Default);
}


AMovingObstacle::AMovingObstacle()
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);
	SetReplicatingMovement(true);
	NetPriority = 2.f;
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
		
		TryMove(NewLocation, GetActorRotation());
	}
}

void AMovingObstacle::PostNetReceiveLocationAndRotation()
{
	const FRepMovement& LocalRepMovement = GetReplicatedMovement();
	const FVector CompensatedLocation = LocalRepMovement.Location + LocalRepMovement.LinearVelocity * GetEstimatedLocalPing() * MovingObstacleCVars::LookaheadFactor;
	const FVector NewLocation = FRepMovement::RebaseOntoLocalOrigin(CompensatedLocation, this);

	if (RootComponent && RootComponent->IsRegistered() && (NewLocation != GetActorLocation() || LocalRepMovement.Rotation != GetActorRotation()))
	{
		const FVector DeltaLocation = NewLocation - GetActorLocation();

		if (DeltaLocation.SizeSquared() > FMath::Square(MovingObstacleCVars::MinimumAdjustment))
		{
			TryMove(NewLocation, LocalRepMovement.Rotation);
		}
	}
}

void AMovingObstacle::TryMove(FVector NewLocation, FRotator NewRotation)
{
	TryPushCharacter(NewLocation - GetActorLocation());
	SetActorLocationAndRotation(NewLocation, NewRotation, /*bSweep=*/ false);

	const UWorld* World = GetWorld();
	const ATetherPrimaryGameMode* GameMode = World ? World->GetAuthGameMode<ATetherPrimaryGameMode>() : nullptr;
	if (HasAuthority() && GameMode)
	{
		const AVolume* ObstacleVolume = GameMode->GetObstacleVolume();
		if (ObstacleVolume && !ObstacleVolume->IsOverlappingActor(this))
		{
			Destroy();
		}
	}
}

void AMovingObstacle::TryPushCharacter(FVector DeltaLocation)
{
	const UWorld* World = GetWorld();
	const UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(RootComponent);
	if (World && PrimitiveComponent)
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
				
			if (bHitPawn && !bIsPawnBase && Character->GetLocalRole() >= ROLE_AutonomousProxy)
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
}

float AMovingObstacle::GetEstimatedLocalPing() const
{
	const UWorld* World = GetWorld();
	const APlayerController* LocalPlayerController = World ? World->GetFirstPlayerController() : nullptr;
	const APlayerState* LocalPlayerState = LocalPlayerController ? LocalPlayerController->GetPlayerState<APlayerState>() : nullptr;

	if (GetNetMode() >= NM_Client && LocalPlayerState)
	{
		return LocalPlayerState->ExactPing / 1000.f;
	}

	return 0.f;
}

