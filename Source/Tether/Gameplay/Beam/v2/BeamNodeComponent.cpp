// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "BeamNodeComponent.h"

#include "BeamManager.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "GeneratedCodeHelpers.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Tether/Tether.h"

UBeamNodeComponent::UBeamNodeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}


void UBeamNodeComponent::BeginPlay()
{
	Super::BeginPlay();
	Register();
}


void UBeamNodeComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	for (int i = 0; i < NodesSupplying.Num() && i < BeamEffects.Num(); i++)
	{
		if (!NodesSupplying[i].IsValid() || !BeamEffects[i])
		{
			continue;
		}
		BeamEffects[i]->SetVariableVec3(TEXT("StartLocation"), GetComponentLocation());
		BeamEffects[i]->SetVariableVec3(TEXT("EndLocation"), NodesSupplying[i]->GetComponentLocation());
	}
}


bool UBeamNodeComponent::CanReachBeam(const UBeamNodeComponent* OtherBeamComponent) const
{
	if (!OtherBeamComponent || !bSendConnections)
	{
		return false;
	}
	const float Distance = FVector::Distance(GetComponentLocation(), OtherBeamComponent->GetComponentLocation());
	if (Distance > Range)
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


void UBeamNodeComponent::SpawnEffectComponent(UBeamNodeComponent* OtherNode)
{
	if (BeamEffect)
	{
		UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BeamEffect, GetComponentLocation());
		Effect->SetVariableVec3(TEXT("StartLocation"), GetComponentLocation());
		Effect->SetVariableVec3(TEXT("EndLocation"), OtherNode->GetComponentLocation());
		BeamEffects.Add(Effect);
	}
}


void UBeamNodeComponent::PowerOn(UBeamNodeComponent* Source, UBeamNodeComponent* Origin, int Iteration)
{
	if (Iteration >= MaxIterations)
	{
		return;
	}
	bPowered = true;
	SetActive(true);
	PowerSource = Source;
	PowerOrigin = Origin;

	if (bSendConnections)
	{
		FindNewNodes(Iteration + 1);
	}
}


void UBeamNodeComponent::PowerOff(int Iteration)
{
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
	for (UNiagaraComponent* Effect : BeamEffects)
	{
		Effect->DestroyInstance();
		Effect->DeactivateImmediate();
		Effect->DestroyComponent();
	}
	NodesSupplying.Empty();
	BeamEffects.Empty();
	bPowered = false;
	PowerSource = nullptr;
	PowerOrigin = nullptr;
	SetActive(bActiveWhenUnpowered);
}


void UBeamNodeComponent::FindNewNodes(int Iteration)
{
	if (!bSendConnections)
	{
		return;
	}
	for (UBeamNodeComponent* Node : GetBeamComponentsInRange(Range))
	{
		if (!Node || Node == this || Node->GetPowered() || !Node->bRecieveConnections || Node == PowerSource)
		{
			continue;
		}
		if (CanReachBeam(Node))
		{
			if (!NodesSupplying.Contains(Node))
			{
				NodesSupplying.Add(Node);
				if (bSelfPowered)
				{
					Node->PowerOn(this, this, Iteration);
					SpawnEffectComponent(Node);
				}
				else
				{
					Node->PowerOn(this, PowerOrigin.Get(), Iteration);
					SpawnEffectComponent(Node);
				}
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
				Node->PowerOff(1);
			}
		}
		else
		{
			NodesPendingDeletion.Insert(i, 0);
		}
	}
	for (int IndexToDelete : NodesPendingDeletion)
	{
		if (NodesSupplying.IsValidIndex(IndexToDelete))
		{
			NodesSupplying.RemoveAt(IndexToDelete);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid node supplying index %d"), IndexToDelete);
		}
		if (BeamEffects.IsValidIndex(IndexToDelete))
		{
			UNiagaraComponent* Effect = BeamEffects[IndexToDelete];
			BeamEffects.RemoveAt(IndexToDelete);
			Effect->DestroyComponent();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid beam effect index %d"), IndexToDelete);
		}
	}
}


FBeamConnection::FBeamConnection()
	:Child(nullptr), Effect(nullptr)
{}


FBeamConnection::FBeamConnection(UBeamNodeComponent* Child, UNiagaraComponent* Effect)
	:Child(Child), Effect(Effect)
{}


