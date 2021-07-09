// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "BeamController.h"
#include "BeamComponent.h"
#include "DrawDebugHelpers.h"
#include "Util/IndexPriorityQueue.h"


ABeamController::ABeamController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ABeamController::BeginPlay()
{
	Super::BeginPlay();

	PrimaryActorTick.TickInterval = TraversalTickInterval;
}

void ABeamController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	TraverseBeams(DeltaSeconds);
}


bool ABeamController::AddBeamTarget(AActor* Target)
{
	UBeamComponent* BeamComponent = Target->Implements<UBeamTarget>() ? IBeamTarget::Execute_GetBeamComponent(Target) : nullptr;

	if (BeamComponent && BeamTargets.AddUnique(Target) != INDEX_NONE)
	{
		BeamComponent->SetStatus(BeamComponent->GetStatus() | EBeamComponentStatus::Connected);
		return true;
	}

	return false;
}

bool ABeamController::RemoveBeamTarget(AActor* Target)
{
	UBeamComponent* BeamComponent = Target->Implements<UBeamTarget>() ? IBeamTarget::Execute_GetBeamComponent(Target) : nullptr;

	if (BeamComponent && BeamComponent->BeamController == this)
	{
		BeamComponent->BeamController = nullptr;
		return BeamTargets.Remove(Target) != 0;
	}

	return false;
}

void ABeamController::UpdateBeamEffects()
{
	// Do something?
}

float ABeamController::CalculateWeightedDistanceCustom_Implementation(FVector StartLocation, FVector EndLocation) const
{
	return FVector::Distance(StartLocation, EndLocation);
}

void ABeamController::HandleTargetDestroyed(AActor* Target)
{
	RemoveBeamTarget(Target);
}

void ABeamController::HandleTargetModeChanged(AActor* Target, EBeamComponentMode OldMode, EBeamComponentMode NewMode)
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

	// Draw debug lines
	if (const UWorld* World = GetWorld())
	{
		for (TPair<int32, int32> PathEdge : PathIndices)
		{
			const FBeamNode& BeamNodeA = BeamNodes[PathEdge.Key];
			const FBeamNode& BeamNodeB = BeamNodes[PathEdge.Value];
			DrawDebugLine(World,
				BeamNodeA.BeamComponent->GetComponentLocation(), BeamNodeB.BeamComponent->GetComponentLocation(),
				FColor::Cyan, false, DeltaTime + 0.05f, 0, 2.f);
		}
	}
}

TArray<ABeamController::FBeamNode> ABeamController::BuildInitialNodes()
{
	static const auto FilterLambda = [](const AActor* Target)
	{
		const UBeamComponent* BeamComponent = Target->Implements<UBeamTarget>() ? IBeamTarget::Execute_GetBeamComponent(Target) : nullptr;

		if (BeamComponent)
		{
			return (BeamComponent->GetMode() & EBeamComponentMode::Connectable) != EBeamComponentMode::None;
		}

		return false;
	};

	TArray<FBeamNode> BeamNodes;

	const UWorld* World = GetWorld();
	if (!World)
	{
		return BeamNodes;
	}
	
	TArray<AActor*> FilteredBeamTargets = BeamTargets.FilterByPredicate(FilterLambda);
	const int32 NumNodes = FilteredBeamTargets.Num();
	BeamNodes.Reserve(NumNodes);

	int32 NumRequiredNodes = 0;

	// Reserve initial data
	for (int32 Index = 0; Index < NumNodes; Index++)
	{
		FBeamNode& NewNode = BeamNodes.Emplace_GetRef();
		NewNode.BeamTarget = FilteredBeamTargets[Index];
		NewNode.NodeDistances.SetNumUninitialized(NumNodes);
		NewNode.BeamComponent = NewNode.BeamTarget->Implements<UBeamTarget>() ? IBeamTarget::Execute_GetBeamComponent(NewNode.BeamTarget) : nullptr;
		NewNode.LinkedRequirement = INDEX_NONE;
		if (ensure(NewNode.BeamComponent))
		{
			NewNode.bRequired = (NewNode.BeamComponent->GetMode() & EBeamComponentMode::Required) != EBeamComponentMode::None;
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

			if (ensure(NodeA.BeamComponent && NodeB.BeamComponent))
			{
				const FVector StartLocation = NodeA.BeamComponent->GetComponentLocation();
				const FVector EndLocation = NodeB.BeamComponent->GetComponentLocation();
				const float PendingDistance = CalculateWeightedDistance(StartLocation, EndLocation);

				if ((MaxNodeDistance < 0.f || PendingDistance < MaxNodeDistance) && !World->LineTraceTestByChannel(StartLocation, EndLocation, BeamTraceChannel))
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
