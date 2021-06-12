// Copyright (c) 2021 Spencer Melnick

#pragma once

#include "CoreMinimal.h"
#include "MovingObstacle.generated.h"


class UArrowComponent;

UCLASS(Blueprintable)
class AMovingObstacle : public AActor
{
	GENERATED_BODY()

public:

	AMovingObstacle();

	virtual void Tick(float DeltaSeconds) override;

};
