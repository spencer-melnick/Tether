// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "BeamManager.h"

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
	
}

// Called every frame
void ABeamManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABeamManager::AddNode(UBeamNodeComponent* Node)
{
	Nodes.Add(TWeakObjectPtr<UBeamNodeComponent>(Node));
}

uint8 ABeamManager::AddUniqueNode(UBeamNodeComponent* Node)
{
	const TWeakObjectPtr<UBeamNodeComponent> NodeRef = TWeakObjectPtr<UBeamNodeComponent>(Node);
	if (!Nodes.Contains(NodeRef))
	{
		UE_LOG(LogTetherGame, Verbose, TEXT("Registered new beam node."));
		return Nodes.Add(NodeRef);
	}
	return -1;
}

