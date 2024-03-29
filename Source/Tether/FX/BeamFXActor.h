// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BeamFXActor.generated.h"


class UBeamComponent;
class ABeamController;


/** Actor spawned by a beam controller to represent a visible beam connection */
UCLASS(Blueprintable, Transient)
class ABeamFXActor : public AActor
{
	GENERATED_BODY()

	friend ABeamController;

public:

	ABeamFXActor();

	// Actor interface
	virtual void Tick(float DeltaSeconds) override;


	// Accessors

	UFUNCTION(BlueprintPure)
	bool IsEffectActive() const { return bEffectActive; }

	UBeamComponent* GetTarget1() const { return Target1; }
	UBeamComponent* GetTarget2() const { return Target2; }

	UFUNCTION(BlueprintPure)
	void GetTargets(UBeamComponent*& OutTarget1, UBeamComponent*& OutTarget2) const;

	FVector GetTargetEffectLocation1() const;
	FVector GetTargetEffectLocation2() const;

	UFUNCTION(BlueprintPure)
	void GetTargetEffectLocations(FVector& OutEffectLocation1, FVector& OutEffectLocation2) const;

	UFUNCTION(BlueprintPure)
	ABeamController* GetOwningBeamController() const;


protected:

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="Set Effect Active"))
	void BP_SetEffectActive(bool bNewEffectActive);

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="Update FX"))
	void BP_UpdateFX(FVector TargetLocation1, FVector TargetLocation2);

private:

	// Beam FX controls

	void SetTargets(UBeamComponent* NewTarget1, UBeamComponent* NewTarget2);
	void ClearTargets();
	void SetEffectActive(bool bNewActive);
	void UpdateFX();


	// State properties

	bool bEffectActive = false;

	UPROPERTY(Transient)
	UBeamComponent* Target1;

	UPROPERTY(Transient)
	UBeamComponent* Target2;
};
