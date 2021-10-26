// Copyright (c) 2021 Spencer Melnick

#pragma once

#include "CoreMinimal.h"

#include "Tether/Core/Suspendable.h"

#include "ObstacleSpawner.generated.h"


class UArrowComponent;
class AMovingObstacle;

UCLASS()
class AObstacleSpawner : public AActor, public ISuspendable
{
	GENERATED_BODY()

public:

	static const FName ArrowComponentName;

	AObstacleSpawner();

	virtual void BeginPlay() override;


	virtual void Suspend() override;
	virtual void Unsuspend() override;

	virtual void CacheInitialState() override;
	virtual void Reload() override;
	
	
	// Editor properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
	FFloatRange SpawnDelay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
	TArray<TSubclassOf<AActor>> ObstacleTypes;

private:

	void SpawnRandomObstacle();

	FTimerHandle ObstacleTimerHandle;

	UPROPERTY()
	UArrowComponent* ArrowComponent;

	UPROPERTY()
	TArray<AActor*> Obstacles;
	
};
