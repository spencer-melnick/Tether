// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "TetherGameModeBase.h"
#include "TetherPrimaryGameMode.generated.h"

UCLASS()
class ATetherPrimaryGameMode : public ATetherGameModeBase
{
	GENERATED_BODY()

public:


	ATetherPrimaryGameMode();


	// Actor overrides

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;


	// Accessors

	const AVolume* GetObstacleVolume() const { return ObstacleVolume; }

	float GetBaseObstacleSpeed(float GamePhaseTime) const;


	// Editor properties

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0"))
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Timing")
	UCurveFloat* ObstacleSpeedCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health", meta=(ClampMin="0.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health", meta=(ClampMin="0.0"))
	float DamagePerSecond;	

private:

	UPROPERTY(Transient)
	AVolume* ObstacleVolume;

};
