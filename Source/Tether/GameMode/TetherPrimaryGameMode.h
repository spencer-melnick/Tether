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

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTetheredChangedDelegate, bool, bNewTethered);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUntetheredTimeElapsed, float, TimeUntethered);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTetherExpired);


	ATetherPrimaryGameMode();


	// Actor overrides

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;


	// Accessors

	const AVolume* GetObstacleVolume() const { return ObstacleVolume; }

	UFUNCTION(BlueprintCallable)
	float GetObstacleSpeed() const;


	// Editor properties

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tether")
	float MaxUntetheredTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tether")
	TEnumAsByte<ECollisionChannel> TetherTraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tether")
	TSubclassOf<AEdisonActor> TetherEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacles")
	float BaseObstacleSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacles")
	UCurveFloat* ObstacleSpeedCurve;


	// Delegates

	UPROPERTY(BlueprintAssignable)
	FTetheredChangedDelegate OnTetheredChanged;

	UPROPERTY(BlueprintAssignable)
	FUntetheredTimeElapsed OnUntetheredTimeElapsed;

	UPROPERTY(BlueprintAssignable)
	FTetherExpired OnTetherExpired;

private:

	// Tether functions

	void SpawnEdisons();
	void CheckAllTethers();
	bool AreCharactersTethered(const ATetherCharacter* FirstCharacter, const ATetherCharacter* SecondCharacter) const;


	// Tether effects

	UPROPERTY(Transient)
	TArray<AEdisonActor*> Edisons;

	UPROPERTY(Transient)
	AVolume* ObstacleVolume;

	TMap<const APlayerController*, TMap<const APlayerController*, AEdisonActor*>> EdisonMap;


	// Tether tracking

	bool bArePlayersTethered = false;

	bool bHaveTethersExpired = false;

	float LastTetheredTime = 0.f;
};
