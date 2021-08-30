// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "BeamController.generated.h"

class UBeamComponent;
class ABeamFXActor;
enum class EBeamComponentMode : uint8;
enum class EBeamComponentStatus : uint8;


/**
 * Defines how distances are computed when calculating the shortest path between targets.
 * (E.g. using the quadratic weighting mode will bias the path solver towards connecting
 * via chains of nearby nodes instead of long singular connections)
 */
UENUM(BlueprintType)
enum class EBeamControllerWeightingMode : uint8
{
	Linear,
	Quadratic,
	Custom
};


/** Internal struct used for tracking FX actor spawning/despawning */
USTRUCT()
struct FBeamControllerFXPoolData
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	ABeamFXActor* FXActor;

	FTimerHandle DespawnTimer;
};


/** Internal struct used for tracking FX edges */
USTRUCT()
struct FBeamFXEdge
{
	GENERATED_BODY()

	FBeamFXEdge()
		: Target1(nullptr), Target2(nullptr)
	{};

	FBeamFXEdge(UBeamComponent* Target1, UBeamComponent* Target2)
		: Target1(Target1), Target2(Target2)
	{};

	UPROPERTY(Transient)
	UBeamComponent* Target1;

	UPROPERTY(Transient)
	UBeamComponent* Target2;
};

bool operator==(const FBeamFXEdge& EdgeA, const FBeamFXEdge& EdgeB);

uint32 GetTypeHash(const FBeamFXEdge& Edge);


/**
 * Actor that creates and manages the lifecycle of beams connecting the player pawns.
 * Intended to be spawned by the game mode
 */
UCLASS(Blueprintable, Transient)
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
	bool AddBeamTarget(UBeamComponent* Target);

	/** Removes a beam target. Returns true if the target was actually removed */
	UFUNCTION(BlueprintCallable, Category="Beam")
	bool RemoveBeamTarget(UBeamComponent* Target);

	/** Function used to calculate weighted distance if mode is set to custom */
	UFUNCTION(BlueprintNativeEvent)
	float CalculateWeightedDistanceCustom(FVector StartLocation, FVector EndLocation) const;
	virtual float CalculateWeightedDistanceCustom_Implementation(FVector StartLocation, FVector EndLocation) const;


	// Event handlers

	void HandleTargetDestroyed(UBeamComponent* Target);
	void HandleTargetModeChanged(UBeamComponent* Target, EBeamComponentMode OldMode, EBeamComponentMode NewMode);


	// Accessors
	
	UFUNCTION(BlueprintPure)
	const TArray<UBeamComponent*>& GetBeamTargets() const { return BeamTargets; }

	/** Returns true if all the beam targets are connected currently */
	UFUNCTION(BlueprintPure, Category="Beam")
	bool AreBeamsConnected() const { return bBeamsConnected; }


	// Editor properties

	/** Maximum distance that will be considered for beam nodes. If less than one then there is no limit */
	UPROPERTY(EditDefaultsOnly)
	float MaxNodeDistance = -1.f;

	/** Trace channel to use for beam node connection */
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ECollisionChannel> BeamTraceChannel;

	UPROPERTY(EditDefaultsOnly)
	EBeamControllerWeightingMode WeightingMode;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ABeamFXActor> BeamFXActorClass;

	/** How long in seconds after deactivation beam FX actors are despawned */
	UPROPERTY(EditDefaultsOnly)
	float BeamFXActorTimeout;
	

private:

	// Path solving

	struct FBeamNode
	{
		UBeamComponent* BeamTarget;
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
	

	// FX control

	void UpdateBeamFX(const TArray<FBeamFXEdge>& BeamEdges);
	
	/** Use this to be able to recycle FX actors instead of constantly deleting and respawning them */
	ABeamFXActor* AcquireBeamFXActor();

	/** Deactivates the visible FX and queues the actor for despawning */
	void DeactivateBeamFXActor(ABeamFXActor* FXActor);

	/** Called when an inactive FX actor's despawn timer expires */
	void HandleBeamFXActorTimeout(ABeamFXActor* FXActor);

	UPROPERTY()
	TArray<UBeamComponent*> BeamTargets;

	UPROPERTY()
	TMap<FBeamFXEdge, ABeamFXActor*> ActiveFXActors;

	UPROPERTY()
	TArray<FBeamControllerFXPoolData> InactiveFXActors;

	bool bBeamsConnected = false;
};
