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

	// Actor overrides
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostNetReceiveLocationAndRotation() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Movement")
	float SpeedMultiplier = 1.f;


protected:

	virtual void TryMove(FVector NewLocation, FRotator NewRotation);
	virtual void TryPushCharacter(FVector DeltaLocation);
	float GetEstimatedLocalPing() const;

};
