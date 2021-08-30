// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "BeamNodeComponent.h"

#include "BeamManager.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "GeneratedCodeHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Tether/Tether.h"

UBeamNodeComponent::UBeamNodeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UBeamNodeComponent::BeginPlay()
{
	Register();
	Super::BeginPlay();
	SetActive(bPowered);
}


void UBeamNodeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (bPowered)
	{
		TryPowerNodes();
		ValidateConnections();
	}
}


bool UBeamNodeComponent::CanReachBeam(const UBeamNodeComponent* OtherBeamComponent) const
{
	if (!OtherBeamComponent)
	{
		return false;
	}
	const float Distance = FVector::Distance(GetComponentLocation(), OtherBeamComponent->GetComponentLocation());
	const float MaxRange = FMath::Max(Range, OtherBeamComponent->Range);
	if (Distance > MaxRange)
	{
		return false;
	}
	if (UWorld* World = GetWorld())
	{
		FHitResult LineTraceHitResult;
		FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;

		bool bResult = !World->LineTraceSingleByChannel(LineTraceHitResult,	GetComponentLocation(), OtherBeamComponent->GetComponentLocation(), BeamTraceChannel, QueryParams);
		return bResult;
	}
	return false;
}


TArray<UBeamNodeComponent*> UBeamNodeComponent::GetBeamComponentsInRange(const float SearchRange) const
{
	TArray<UBeamNodeComponent*> ResultComponents;
	if (AActor* Manager = UGameplayStatics::GetActorOfClass(this, ABeamManager::StaticClass()))
	{
		if (ABeamManager* BeamManager = Cast<ABeamManager>(Manager))
		{
			for (TWeakObjectPtr<UBeamNodeComponent> BeamNodeComponent : BeamManager->GetNodes())
			{
				if (BeamNodeComponent.Get() == this)
				{
					continue;
				}
				ResultComponents.Add(BeamNodeComponent.Get());
			}
		}
	}
	return ResultComponents;
}


void UBeamNodeComponent::Register()
{
	if (AActor* Manager = UGameplayStatics::GetActorOfClass(this, ABeamManager::StaticClass()))
	{
		if (ABeamManager* BeamManager = Cast<ABeamManager>(Manager))
		{
			Id = BeamManager->AddUniqueNode(this);
		}
		else
		{
			UE_LOG(LogTetherGame, Warning, TEXT("Beam Node was spawned, but a Beam Manager could not be found."));
		}
	}
}


void UBeamNodeComponent::PowerOn(UBeamNodeComponent* Source, UBeamNodeComponent* Origin, int Iteration)
{
	if (Iteration >= MaxIterations)
	{
		return;
	}
	// UE_LOG(LogTetherGame, Verbose, TEXT("Beam node %i turned on by node %i... Stack iteration %i"), Id, Source->GetId(), Iteration);
	bPowered = true;
	SetActive(true);
	PowerSource = Source;
	PowerOrigin = Origin;
	
	TryPowerNodes(Iteration + 1);
}


void UBeamNodeComponent::PowerOff(int Iteration)
{
	if (Iteration >= MaxIterations)
	{
		return;
	}
	for (TWeakObjectPtr<UBeamNodeComponent> Node : NodesSupplying)
	{
		// UE_LOG(LogTetherGame, Verbose, TEXT("Beam node %i turned off by %i... Stack iteration %i"), Node->GetId(), Id, Iteration);
		Node->PowerOff(Iteration + 1);
	}
	NodesSupplying.Empty();
	bPowered = false;
	SetActive(false);
	PowerSource = nullptr;
	PowerOrigin = nullptr;
}


void UBeamNodeComponent::TryPowerNodes(int Iteration)
{
	for (UBeamNodeComponent* Node : GetBeamComponentsInRange(Range))
	{
		if(Node == this || Node->GetPowered() || Node == PowerSource)
		{
			continue;
		}
		if (CanReachBeam(Node))
		{
			DrawDebugLine(GetWorld(), GetComponentLocation(), Node->GetComponentLocation(), FColor::Blue, false, 5.0f, 1, 5.0f);
			NodesSupplying.AddUnique(Node);
			Node->PowerOn(this, PowerOrigin.Get(), Iteration);
		}
	}
}


void UBeamNodeComponent::ValidateConnections()
{
	TArray<int> NodesPendingDeletion;
	for (int i = 0; i < NodesSupplying.Num(); i++)
	{
		UBeamNodeComponent* Node = NodesSupplying[i].Get();
		if (!CanReachBeam(Node))
		{
			NodesPendingDeletion.Insert(i, 0);
		}
	}
	for (int IndexToDelete : NodesPendingDeletion)
	{
		NodesSupplying[IndexToDelete]->PowerOff();
		NodesSupplying.RemoveAt(IndexToDelete);
	}
}
