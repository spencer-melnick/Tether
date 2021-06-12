// Copyright (c) 2021 Spencer Melnick

#include "ObstacleSpawner.h"

#include "Components/ArrowComponent.h"
#include "MovingObstacle.h"


const FName AObstacleSpawner::ArrowComponentName(TEXT("ArrowComponent"));

AObstacleSpawner::AObstacleSpawner()
{
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(ArrowComponentName);
	ArrowComponent->SetupAttachment(RootComponent);
}

void AObstacleSpawner::BeginPlay()
{
	Super::BeginPlay();

	SpawnRandomObstacle();
}

void AObstacleSpawner::SpawnRandomObstacle()
{
	UWorld* World = GetWorld();
	if (World && ObstacleTypes.Num() > 0)
	{
		const int32 ObstacleTypeIndex = FMath::RandRange(0, ObstacleTypes.Num() - 1);
		const TSubclassOf<AMovingObstacle> ObstacleType = ObstacleTypes[ObstacleTypeIndex];

		World->SpawnActor<AMovingObstacle>(ObstacleType, GetActorLocation(), GetActorRotation());
		
		World->GetTimerManager().SetTimer(ObstacleTimerHandle,
            this, &AObstacleSpawner::SpawnRandomObstacle,
            FMath::RandRange(SpawnDelay.GetLowerBoundValue(), SpawnDelay.GetUpperBoundValue()));
	}
}

