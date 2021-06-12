// Copyright (c) 2021 Spencer Melnick

#pragma once

#include "CoreMinimal.h"
#include "ObstacleSpawner.generated.h"


class UArrowComponent;
class AMovingObstacle;

UCLASS()
class AObstacleSpawner : public AActor
{
	GENERATED_BODY()

public:

	static const FName ArrowComponentName;

	AObstacleSpawner();

	virtual void BeginPlay() override;


	// Editor properties

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
	FFloatRange SpawnDelay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
	TArray<TSubclassOf<AMovingObstacle>> ObstacleTypes;

private:

	void SpawnRandomObstacle();


	FTimerHandle ObstacleTimerHandle;

	UPROPERTY()
	UArrowComponent* ArrowComponent;
	
};
