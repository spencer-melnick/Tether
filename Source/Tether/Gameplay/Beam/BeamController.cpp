// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "BeamController.h"
#include "BeamComponent.h"
#include "DrawDebugHelpers.h"
#include "Tether/FX/BeamFXActor.h"
#include "Util/IndexPriorityQueue.h"


namespace BeamControllerCVars
{
	float BeamFXTimeout = 10.f;
	FAutoConsoleVariableRef CVarBeamFXTimeout(
		TEXT("BeamController.BeamFXTimeout"), BeamFXTimeout,
		TEXT("How long (in seconds) after deactivation a beam FX actor will be despawned"),
		ECVF_Default);

	bool bDrawDebugConnections = false;
	FAutoConsoleVariableRef CVarDrawDebugConnections(
		TEXT("BeamController.DrawDebugConnections"), bDrawDebugConnections,
		TEXT("If true the beam controller will draw simple debug lines showing all active connectons"),
		ECVF_Default);
}


bool operator==(const FBeamFXEdge& EdgeA, const FBeamFXEdge& EdgeB)
{
	return
		(EdgeA.Target1 == EdgeB.Target1 && EdgeA.Target2 == EdgeB.Target2) ||
		(EdgeA.Target1 == EdgeB.Target2 && EdgeA.Target2 == EdgeB.Target1);
}

uint32 GetTypeHash(const FBeamFXEdge& Edge)
{
	return GetTypeHash(Edge.Target1) ^ GetTypeHash(Edge.Target2);
}

ABeamController::ABeamController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ABeamController::BeginPlay()
{
	Super::BeginPlay();
}

void ABeamController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	TraverseBeams(DeltaSeconds);
}


bool ABeamController::AddBeamTarget(UBeamComponent* Target)
{
	if (Target && BeamTargets.AddUnique(Target) != INDEX_NONE)
	{
		Target->SetStatus(Target->GetStatus() | EBeamComponentStatus::Tracked);
		return true;
	}

	return false;
}

bool ABeamController::RemoveBeamTarget(UBeamComponent* Target)
{
	if (Target && Target->BeamController == this)
	{
		Target->BeamController = nullptr;
		return BeamTargets.Remove(Target) != 0;
	}

	return false;
}

float ABeamController::CalculateWeightedDistanceCustom_Implementation(FVector StartLocation, FVector EndLocation) const
{
	return FVector::Distance(StartLocation, EndLocation);
}

void ABeamController::HandleTargetDestroyed(UBeamComponent* Target)
{
	RemoveBeamTarget(Target);
}

void ABeamController::HandleTargetModeChanged(UBeamComponent* Target, EBeamComponentMode OldMode, EBeamComponentMode NewMode)
{
	// Do something
}

void ABeamController::TraverseBeams(float DeltaTime)
{
	TArray<FBeamNode> BeamNodes = BuildInitialNodes();

	if (BeamNodes.Num() < 2)
	{
		// We need at least two nodes to do any tests
		return;
	}
	
	TArray<int32> RequiredNodeIndices;
	for (int32 Index = 0; Index < BeamNodes.Num(); Index++)
	{
		if (BeamNodes[Index].bRequired)
		{
			RequiredNodeIndices.Add(Index);
		}
	}

	if (!ensure(RequiredNodeIndices.Num() >= 2))
	{
		return;
	}

	TArray<TPair<int32, int32>> PathIndices;
	TSet<int32> EndIndices;
	bool bAllNodesLinked = true;
	
	for (int32 RequiredNodeIndex : RequiredNodeIndices)
	{
		if (EndIndices.Contains(RequiredNodeIndex))
		{
			continue;
		}
		
		FindLinkedNodes(BeamNodes, RequiredNodeIndex, PathIndices, EndIndices);

		// Check if all of the required nodes are present
		if (EndIndices.Num() < RequiredNodeIndices.Num())
		{
			bAllNodesLinked = false;
		}
	}

	// Gather beam edges
	TArray<FBeamFXEdge> BeamEdges;
	BeamEdges.Reserve(PathIndices.Num());

	if (const UWorld* World = GetWorld())
	{
		for (const TPair<int32, int32> PathEdge : PathIndices)
		{
			const FBeamNode& BeamNodeA = BeamNodes[PathEdge.Key];
			const FBeamNode& BeamNodeB = BeamNodes[PathEdge.Value];

			BeamEdges.Emplace(BeamNodeA.BeamTarget, BeamNodeB.BeamTarget);

			if (BeamControllerCVars::bDrawDebugConnections)
			{
				DrawDebugLine(World,
					BeamNodeA.BeamTarget->GetComponentLocation(), BeamNodeB.BeamTarget->GetComponentLocation(),
					FColor::Cyan, false, DeltaTime + 0.05f, 0, 2.f);
			}
		}
	}

	UpdateBeamFX(BeamEdges);
}


TArray<ABeamController::FBeamNode> ABeamController::BuildInitialNodes()
{
	static const auto FilterLambda = [](const UBeamComponent* Target)
	{
		return (Target->GetMode() & EBeamComponentMode::Connectable) != EBeamComponentMode::None;
	};
	TArray<FBeamNode> BeamNodes;
	const UWorld* World = GetWorld();
	if (!World)
	{
		return BeamNodes;
	}
	
	TArray<UBeamComponent*> FilteredBeamTargets = BeamTargets.FilterByPredicate(FilterLambda);
	const int32 NumNodes = FilteredBeamTargets.Num();
	BeamNodes.Reserve(NumNodes);

	int32 NumRequiredNodes = 0;

	// Reserve initial data
	for (int32 Index = 0; Index < NumNodes; Index++)
	{
		FBeamNode& NewNode = BeamNodes.Emplace_GetRef();
		NewNode.BeamTarget = FilteredBeamTargets[Index];
		NewNode.NodeDistances.SetNumUninitialized(NumNodes);
		NewNode.LinkedRequirement = INDEX_NONE;
		if (ensure(NewNode.BeamTarget))
		{
			NewNode.bRequired = (NewNode.BeamTarget->GetMode() & EBeamComponentMode::Required) != EBeamComponentMode::None;
			NewNode.bConnected = NewNode.bRequired;
			NumRequiredNodes++;
		}
	}

	if (NumRequiredNodes < 2)
	{
		// If there are less than two required nodes than we can't test anything
		BeamNodes.Reset();
		return BeamNodes;
	}

	// Compute distances and traces
	for (int32 IndexI = 0; IndexI < NumNodes; IndexI++)
	{
		FBeamNode& NodeA = BeamNodes[IndexI];
		NodeA.NodeDistances[IndexI] = -1.f;
		
		for (int32 IndexJ = IndexI + 1; IndexJ < NumNodes; IndexJ++)
		{
			FBeamNode& NodeB = BeamNodes[IndexJ];
			float NodeDistance = -1.f;

			if (ensure(NodeA.BeamTarget && NodeB.BeamTarget))
			{
				const FVector StartLocation = NodeA.BeamTarget->GetComponentLocation();
				const FVector EndLocation = NodeB.BeamTarget->GetComponentLocation();
				const float RealDistance = FVector::Distance(StartLocation, EndLocation);
				const float PendingDistance = CalculateWeightedDistance(StartLocation, EndLocation);

				if ((MaxNodeDistance < 0.f || RealDistance < MaxNodeDistance) && !World->LineTraceTestByChannel(StartLocation, EndLocation, BeamTraceChannel))
				{
					NodeDistance = PendingDistance;
				}
			}

			NodeA.NodeDistances[IndexJ] = NodeDistance;
			NodeB.NodeDistances[IndexI] = NodeDistance;
		}
	}

	return BeamNodes;
}


void ABeamController::FindLinkedNodes(const TArray<FBeamNode>& BeamNodes, int32 StartingIndex, TArray<TPair<int32, int32>>& OutPath, TSet<int32>& OutEndIndices)
{
	if (!ensure(BeamNodes.IsValidIndex(StartingIndex)))
	{
		return;
	}

	// Reserve memory for all of our data structures
	TMap<int32, float> VisitedDistances;
	VisitedDistances.Reserve(BeamNodes.Num());
	VisitedDistances.Emplace(StartingIndex, 0.f);

	TMap<int32, int32> PreviousNodes;
	PreviousNodes.Reserve(BeamNodes.Num());

	FIndexPriorityQueue EdgeNodes;
	EdgeNodes.Initialize(BeamNodes.Num());
	EdgeNodes.Insert(StartingIndex, 0.f);

	TSet<int32> EndIndices;

	while (EdgeNodes.GetCount() > 0)
	{
		const int32 CurrentIndex = EdgeNodes.Dequeue();
		const FBeamNode& CurrentNode = BeamNodes[CurrentIndex];

		if (CurrentNode.bRequired)
		{
			EndIndices.Add(CurrentIndex);
		}

		// Analyze all adjacent nodes
		for (int32 AdjacentIndex = 0; AdjacentIndex < BeamNodes.Num(); AdjacentIndex++)
		{
			if (!ensure(CurrentNode.NodeDistances.IsValidIndex(AdjacentIndex)) || CurrentNode.NodeDistances[AdjacentIndex] < 0.f)
			{
				continue;
			}

			// If the new path is shorter, add it to our pending edge nodes
			const float PendingDistance = VisitedDistances[CurrentIndex] + CurrentNode.NodeDistances[AdjacentIndex];
			if (!VisitedDistances.Contains(AdjacentIndex) || VisitedDistances[AdjacentIndex] > PendingDistance)
			{
				VisitedDistances.Emplace(AdjacentIndex, PendingDistance);
				PreviousNodes.Emplace(AdjacentIndex, CurrentIndex);
				EdgeNodes.Insert(AdjacentIndex, PendingDistance);
			}
		}
	}

	// If we've found an end, traverse backwards and build the path
	for (int32 EndIndex : EndIndices)
	{
		for (int32 PathIndex = EndIndex; PathIndex != StartingIndex; PathIndex = PreviousNodes[PathIndex])
		{
			if (OutPath.AddUnique(TPair<int32, int32>(PathIndex, PreviousNodes[PathIndex])) == INDEX_NONE)
			{
				break;
			}
			BeamTargets[PreviousNodes[PathIndex]]->SetStatus(EBeamComponentStatus::Connected);
		}
	}

	OutEndIndices.Append(EndIndices);
}


float ABeamController::CalculateWeightedDistance(FVector StartLocation, FVector EndLocation) const
{
	switch (WeightingMode)
	{
		case EBeamControllerWeightingMode::Linear:
			return FVector::Distance(StartLocation, EndLocation);

		case EBeamControllerWeightingMode::Quadratic:
			return FVector::DistSquared(StartLocation, EndLocation);

		default:
			return CalculateWeightedDistanceCustom(StartLocation, EndLocation);
	}
}


void ABeamController::UpdateBeamFX(const TArray<FBeamFXEdge>& BeamEdges)
{
	TArray<FBeamFXEdge> EdgesPendingRemoval;
	TArray<FBeamFXEdge> EdgesPendingCreation;
	
	for (const TPair<FBeamFXEdge, ABeamFXActor*> ActiveFXActor : ActiveFXActors)
	{
		// Clear out any edges we don't have anymore
		if (!BeamEdges.Contains(ActiveFXActor.Key))
		{
			EdgesPendingRemoval.AddUnique(ActiveFXActor.Key);
			ActiveFXActor.Key.Target1->SetStatus(EBeamComponentStatus::Tracked);
			ActiveFXActor.Key.Target2->SetStatus(EBeamComponentStatus::Tracked);
		}
	}

	for (const FBeamFXEdge& PendingEdge : BeamEdges)
	{
		if (!ActiveFXActors.Contains(PendingEdge))
		{
			EdgesPendingCreation.AddUnique(PendingEdge);

		}
	}

	for (const FBeamFXEdge& RemovedEdge : EdgesPendingRemoval)
	{
		if (ABeamFXActor* RemovedFXActor = ActiveFXActors.FindAndRemoveChecked(RemovedEdge))
		{
			DeactivateBeamFXActor(RemovedFXActor);
		}
	}

	for (const FBeamFXEdge& AddedEdge : EdgesPendingCreation)
	{
		ABeamFXActor* NewFXActor = AcquireBeamFXActor();
		if (ensure(NewFXActor))
		{
			NewFXActor->SetTargets(AddedEdge.Target1, AddedEdge.Target2);
			NewFXActor->SetEffectActive(true);
			NewFXActor->UpdateFX();
			ActiveFXActors.Add(AddedEdge, NewFXActor);
		}
	}
}


ABeamFXActor* ABeamController::AcquireBeamFXActor()
{
	UWorld* World = GetWorld();
	if (ensure(World))
	{
		ABeamFXActor* NewFXActor = nullptr;

		if (InactiveFXActors.Num() > 0)
		{
			FBeamControllerFXPoolData PoolData = InactiveFXActors.Pop();
			World->GetTimerManager().ClearTimer(PoolData.DespawnTimer);
			NewFXActor = PoolData.FXActor;
		}
		else
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			NewFXActor = World->SpawnActor<ABeamFXActor>(BeamFXActorClass, SpawnParameters);
		}

		if (ensure(NewFXActor))
		{
			NewFXActor->ClearTargets();
		}

		return NewFXActor;
	}

	return nullptr;
}

void ABeamController::DeactivateBeamFXActor(ABeamFXActor* FXActor)
{
	const UWorld* World = GetWorld();
	if (ensure(FXActor && World))
	{
		FXActor->ClearTargets();
		FXActor->SetEffectActive(false);

		FBeamControllerFXPoolData& PoolData = InactiveFXActors.AddDefaulted_GetRef();
		PoolData.FXActor = FXActor;
		World->GetTimerManager().SetTimer(PoolData.DespawnTimer,
			FTimerDelegate::CreateUObject(this, &ABeamController::HandleBeamFXActorTimeout, FXActor),
			BeamFXActorTimeout, false);
	}
}

void ABeamController::HandleBeamFXActorTimeout(ABeamFXActor* FXActor)
{
	const UWorld* World = GetWorld();
	if (FXActor && ensure(World))
	{
		InactiveFXActors.RemoveAllSwap([World, FXActor](FBeamControllerFXPoolData& PoolData)
		{
			if (PoolData.FXActor == FXActor)
			{
				World->GetTimerManager().ClearTimer(PoolData.DespawnTimer);
				PoolData.FXActor->Destroy();
				return true;
			}

			return false;
		});
	}
}
