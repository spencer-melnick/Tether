// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "BeamManager.h"

#include "BeamBatteryComponent.h"
#include "Tether/Tether.h"


// Sets default values
ABeamManager::ABeamManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ABeamManager::BeginPlay()
{
	Super::BeginPlay();
	PrimaryActorTick.UpdateTickIntervalAndCoolDown(TickInterval);
}

// Called every frame
void ABeamManager::Tick(float DeltaTime)
{
	Cleanup();
	for (TWeakObjectPtr<UBeamNodeComponent> Node : Nodes)
	{
		if (Node->GetPowered())
		{
			Node->ValidateConnections();
		}
	}
	for (TWeakObjectPtr<UBeamNodeComponent> Node : Nodes)
	{
		if (Node->GetPowered())
		{
			Node->FindNewNodes(0);
		}
	}
	for (TWeakObjectPtr<UBeamNodeComponent> Node : Nodes)
	{
		if (UBeamBatteryComponent* Battery = Cast<UBeamBatteryComponent>(Node.Get()))
		{
			Battery->PowerReceivers(DeltaTime);
		}
	}
}

void ABeamManager::AddNode(UBeamNodeComponent* Node)
{
	Nodes.Add(TWeakObjectPtr<UBeamNodeComponent>(Node));
}

uint8 ABeamManager::AddUniqueNode(UBeamNodeComponent* Node)
{
	Cleanup();
	const TWeakObjectPtr<UBeamNodeComponent> NodeRef = TWeakObjectPtr<UBeamNodeComponent>(Node);
	if (!Nodes.Contains(NodeRef))
	{
		// UE_LOG(LogTetherGame, Verbose, TEXT("Registered new beam node."));
		return Nodes.Add(NodeRef);
	}
	return -1;
}

void ABeamManager::Cleanup()
{
	TArray<int> IndicesToDelete;
	for (int i = 0; i < Nodes.Num(); i++)
	{
		if (Nodes[i].IsStale())
		{
			IndicesToDelete.Insert(i, 0);
		}
	}
	for (int IndexToDelete : IndicesToDelete)
	{
		Nodes.RemoveAt(IndexToDelete);
	}
}


float ABeamManager::GetTickInterval() const
{
	return TickInterval;
}
