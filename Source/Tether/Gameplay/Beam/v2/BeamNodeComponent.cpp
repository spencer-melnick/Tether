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
	Super::BeginPlay();
	Register();
	SetActive(bPowered || bSelfPowered);
}


void UBeamNodeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (bSendConnections && bPowered || bSelfPowered)
	{
		if (PowerSource.IsValid() || bSelfPowered)
		{
			TryPowerNodes();
			ValidateConnections();
		}
		else
		{
			PowerSource = nullptr;
			PowerOff();
		}
	}
}

void UBeamNodeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	PowerOff();
}


void UBeamNodeComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (bSendConnections)
	{
		for (TWeakObjectPtr<UBeamNodeComponent> Node : NodesSupplying)
		{
			if(Node.IsValid())
			{
				Node->PowerOff();
			}
		}
	}
	Super::OnComponentDestroyed(bDestroyingHierarchy);
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
			PrimaryComponentTick.TickInterval = BeamManager->GetTickInterval();
		}
		else
		{
			UE_LOG(LogTetherGame, Warning, TEXT("Beam Node was spawned, but a Beam Manager could not be found."));
		}
	}
}


void UBeamNodeComponent::PowerOn(UBeamNodeComponent* Source, UBeamNodeComponent* Origin, int Iteration)
{
	if (Iteration >= MaxIterations || !bRecieveConnections)
	{
		return;
	}
	bPowered = true;
	SetActive(true);
	PowerSource = Source;
	PowerOrigin = Origin;

	if (bSendConnections)
	{
		TryPowerNodes(Iteration + 1);
	}
}


void UBeamNodeComponent::PowerOff(int Iteration)
{
	if (!this)
	{
		UE_LOG(LogTetherGame, Error, TEXT("A node lost itself..."));
		return;
	}
	if (Iteration >= MaxIterations)
	{
		return;
	}
	for (TWeakObjectPtr<UBeamNodeComponent> Node : NodesSupplying)
	{
		if (Node.IsValid())
		{
			Node->PowerOff(Iteration + 1);
		}
	}
	NodesSupplying.Empty();
	bPowered = false;
	PowerSource = nullptr;
	PowerOrigin = nullptr;
	SetActive(bActiveWhenUnpowered);
}


void UBeamNodeComponent::TryPowerNodes(int Iteration)
{
	for (UBeamNodeComponent* Node : GetBeamComponentsInRange(Range))
	{
		if (!Node || Node == this || Node->GetPowered() || !Node->bRecieveConnections || Node == PowerSource)
		{
			continue;
		}
		if (CanReachBeam(Node))
		{
			NodesSupplying.AddUnique(Node);
			if (bSelfPowered)
			{
				Node->PowerOn(this, this, Iteration);
			}
			else
			{
				Node->PowerOn(this, PowerOrigin.Get(), Iteration);
			}
		}
	}
}


void UBeamNodeComponent::ValidateConnections()
{
	TArray<int> NodesPendingDeletion;
	for (int i = 0; i < NodesSupplying.Num(); i++)
	{
		UBeamNodeComponent* Node = NodesSupplying[i].Get();
		if (Node)
		{
			if(!CanReachBeam(Node))
			{
				NodesPendingDeletion.Insert(i, 0);
				NodesSupplying[i]->PowerOff();
			}
			DrawDebugLine(GetWorld(), GetComponentLocation(), Node->GetComponentLocation(), FColor::Blue, true, 5.0f, 1, 5.0f);
		}
		else
		{
			NodesPendingDeletion.Insert(i, 0);
		}
	}
	for (int IndexToDelete : NodesPendingDeletion)
	{
		NodesSupplying.RemoveAt(IndexToDelete);
	}
}
