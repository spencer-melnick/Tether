// Copyright (c) 2021 Spencer Melnick

#include "MovingObstacle.h"

#include "Tether/Gamemode/TetherPrimaryGameMode.h"

AMovingObstacle::AMovingObstacle()
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AMovingObstacle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	const UWorld* World = GetWorld();
	const ATetherPrimaryGameMode* GameMode = World ? World->GetAuthGameMode<ATetherPrimaryGameMode>() : nullptr;
	if (GameMode)
	{
		const FVector NewLocation = GetActorLocation() + GetActorForwardVector() * GameMode->GetObstacleSpeed() * SpeedMultiplier * DeltaSeconds;
		const AVolume* ObstacleVolume = GameMode->GetObstacleVolume();
		
		if (ObstacleVolume && !ObstacleVolume->IsOverlappingActor(this))
		{
			Destroy();
		}
		else
		{
			SetActorLocation(NewLocation);
		}
	}
}

