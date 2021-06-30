// Copyright (c) 2021 Spencer Melnick

#include "MovingObstacle.h"

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
		const FVector NewLocation = GetActorLocation() + GetActorForwardVector() * GameState->GetBaseObstacleSpeed() * SpeedMultiplier * DeltaSeconds;
		SetActorLocation(NewLocation, true);
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

