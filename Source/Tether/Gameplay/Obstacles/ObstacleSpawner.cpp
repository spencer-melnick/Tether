// Copyright (c) 2021 Spencer Melnick

#include "ObstacleSpawner.h"

#include "Components/ArrowComponent.h"


const FName AObstacleSpawner::ArrowComponentName(TEXT("ArrowComponent"));

AObstacleSpawner::AObstacleSpawner()
{
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(ArrowComponentName);
	ArrowComponent->SetupAttachment(RootComponent);
}

void AObstacleSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// Do not spawn object until after the first timer has completed...
		if (UWorld* World = GetWorld())
		{
			// Always be the shortest spawn time for the first obstacle
			World->GetTimerManager().SetTimer(ObstacleTimerHandle,
			this, &AObstacleSpawner::SpawnRandomObstacle,
			0.01f);
		}
	}
}


void AObstacleSpawner::Suspend()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().PauseTimer(ObstacleTimerHandle);
	}
	for (AActor* Obstacle : Obstacles)
	{
		if (!Obstacle)
		{
			continue;
		}
		for (UActorComponent* ActorComponent : Obstacle->GetComponents())
		{
			ActorComponent->SetComponentTickEnabled(false);
		}
	}
}

void AObstacleSpawner::Unsuspend()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().UnPauseTimer(ObstacleTimerHandle);
	}
	for (AActor* Obstacle : Obstacles)
	{
		if (!Obstacle)
		{
			continue;
		}
		for (UActorComponent* ActorComponent : Obstacle->GetComponents())
		{
			ActorComponent->SetComponentTickEnabled(true);
		}
	}
}


void AObstacleSpawner::CacheInitialState()
{
	// No initial state necessary for right now
}


void AObstacleSpawner::Reload()
{
	for (AActor* Obstacle : Obstacles)
	{
		if (Obstacle)
		{
			Obstacle->Destroy();
		}
	}
}


void AObstacleSpawner::SpawnRandomObstacle()
{
	UWorld* World = GetWorld();
	if (World && ObstacleTypes.Num() > 0)
	{
		const int32 ObstacleTypeIndex = FMath::RandRange(0, ObstacleTypes.Num() - 1);
		const TSubclassOf<AActor> ObstacleType = ObstacleTypes[ObstacleTypeIndex];

		if (!ensure(ObstacleType))
		{
			return;
		}

		AActor* Obstacle = World->SpawnActor<AActor>(ObstacleType, GetActorLocation(), GetActorRotation());
		if (!IsValid(Obstacle))
		{
			return;
		}
		Obstacle->SetLifeSpan(50.0f);
		Obstacle->Tags.Add(TEXT("Obstacle"));
		Obstacles.Add(Obstacle);
		
		World->GetTimerManager().SetTimer(ObstacleTimerHandle,
            this, &AObstacleSpawner::SpawnRandomObstacle,
            FMath::RandRange(SpawnDelay.GetLowerBoundValue(), SpawnDelay.GetUpperBoundValue()));
	}
}

