// Copyright (c) 2021 Spencer Melnick

#include "MovingObstacle.h"


#include "Components/BrushComponent.h"
#include "Tether/TetherGameModeBase.h"

AMovingObstacle::AMovingObstacle()
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AMovingObstacle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	const UWorld* World = GetWorld();
	const ATetherGameModeBase* GameMode = World ? World->GetAuthGameMode<ATetherGameModeBase>() : nullptr;
	if (GameMode)
	{
		const FVector NewLocation = GetActorLocation() + GetActorForwardVector() * GameMode->GetObstacleSpeed() * DeltaSeconds;
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

