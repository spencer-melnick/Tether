// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "BeamController.generated.h"

class UBeamComponent;
enum class EBeamComponentMode : uint8;
enum class EBeamComponentStatus : uint8;


UENUM(BlueprintType)
enum class EBeamControllerWeightingMode : uint8
{
	Linear,
	Quadratic,
	Custom
};


/**
 * Actor that creates and manages the lifecycle of beams connecting the player pawns.
 * Intended to be spawned by the game mode
 */
UCLASS(Blueprintable)
class ABeamController : public AInfo
{
	GENERATED_BODY()

public:

	ABeamController();


	// Actor interface

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;


	// Beam controls

	/**
	 * Adds a beam target and spawns effects as necessary. Does not do anything if the target actor does not implement
	 * IBeamTarget or is already tracked. Returns true if the target was actually added.
	 */
	UFUNCTION(BlueprintCallable, Category="Beam")
	bool AddBeamTarget(AActor* Target);

	/** Removes a beam target. Returns true if the target was actually removed */
	UFUNCTION(BlueprintCallable, Category="Beam")
	bool RemoveBeamTarget(AActor* Target);

	/** Updates the state of any spawned beam effects */
	UFUNCTION(BlueprintCallable, Category="Beam")
	void UpdateBeamEffects();

	/** Function used to calculate weighted distance if mode is set to custom */
	UFUNCTION(BlueprintNativeEvent)
	float CalculateWeightedDistanceCustom(FVector StartLocation, FVector EndLocation) const;
	virtual float CalculateWeightedDistanceCustom_Implementation(FVector StartLocation, FVector EndLocation) const;


	// Event handlers

	void HandleTargetDestroyed(AActor* Target);
	void HandleTargetModeChanged(AActor* Target, EBeamComponentMode OldMode, EBeamComponentMode NewMode);


	// Accessors
	
	UFUNCTION(BlueprintPure)
	const TArray<AActor*>& GetBeamTargets() const { return BeamTargets; }

	/** Returns true if all the beam targets are connected currently */
	UFUNCTION(BlueprintCallable, Category="Beam")
	bool AllBeamsConnected() const { return bBeamsConnected; }


	// Editor properties

	/** Maximum distance that will be considered for beam nodes. If less than one then there is no limit */
	UPROPERTY(EditDefaultsOnly)
	float MaxNodeDistance = -1.f;

	/** Trace channel to use for beam node connection */
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ECollisionChannel> BeamTraceChannel;

	/** Amount of time (in seconds) between checking all beam connections */
	UPROPERTY(EditDefaultsOnly)
	float TraversalTickInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly)
	EBeamControllerWeightingMode WeightingMode;
	

private:

	struct FBeamNode
	{
		AActor* BeamTarget;
		UBeamComponent* BeamComponent;
		TArray<float> NodeDistances;
		int32 LinkedRequirement;
		uint8 bRequired : 1;
		uint8 bConnected : 1;
	};

	/** Traverse all of the potential beam connections tracked by this controller, updating state as necessary */
	void TraverseBeams(float DeltaTime);

	/** Build an array of connected beam nodes from all active tracked targets */
	TArray<FBeamNode> BuildInitialNodes();

	/** Returns the path from the starting node to the nearest connected node as index pairs */
	void FindLinkedNodes(const TArray<FBeamNode>& BeamNodes, int32 StartingIndex, TArray<TPair<int32, int32>>& OutPath, TSet<int32>& OutEndIndices);

	/** Calculate the weighted distance between targets based on the selected weighting mode */
	float CalculateWeightedDistance(FVector StartLocation, FVector EndLocation) const;

	UPROPERTY()
	TArray<AActor*> BeamTargets;

	bool bBeamsConnected = false;
};
