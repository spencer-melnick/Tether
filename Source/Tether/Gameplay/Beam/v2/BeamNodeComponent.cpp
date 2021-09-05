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

FBeamConnection::FBeamConnection()
	:Child(nullptr), Effect(nullptr)
{}


FBeamConnection::FBeamConnection(UBeamNodeComponent* Child, UNiagaraComponent* Effect)
	:Child(Child), Effect(Effect)
{}


UBeamNodeComponent::UBeamNodeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.0f;
}


void UBeamNodeComponent::BeginPlay()
{
	Super::BeginPlay();
	Register();
}


void UBeamNodeComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	for (int i = 0; i < NodesSupplying.Num(); i++)
	{
		if (!NodesSupplying[i].Child || !NodesSupplying[i].Effect)
		{
			continue;
		}
		NodesSupplying[i].Effect->SetVariableVec3(TEXT("StartLocation"), GetComponentLocation());
		NodesSupplying[i].Effect->SetVariableVec3(TEXT("EndLocation"), NodesSupplying[i].Child->GetComponentLocation());
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
	if (Manager)
	{
		for (TWeakObjectPtr<UBeamNodeComponent> BeamNodeComponent : Manager->GetNodes())
		{
			if (BeamNodeComponent.Get() == this)
			{
				continue;
			}
			ResultComponents.Add(BeamNodeComponent.Get());
		}
	}
	return ResultComponents;
}


void UBeamNodeComponent::Register()
{
	if (AActor* GenericManager = UGameplayStatics::GetActorOfClass(this, ABeamManager::StaticClass()))
	{
		if (ABeamManager* BeamManager = Cast<ABeamManager>(GenericManager))
		{
			Manager = BeamManager;
			Id = BeamManager->AddUniqueNode(this);
		}
		else
		{
			UE_LOG(LogTetherGame, Warning, TEXT("Beam Node was spawned, but a Beam Manager could not be found."));
		}
	}
}


UNiagaraComponent* UBeamNodeComponent::SpawnEffectComponent(UBeamNodeComponent* OtherNode) const
{
	if (BeamEffect)
	{
		UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BeamEffect, GetComponentLocation());
		Effect->SetVariableVec3(TEXT("StartLocation"), GetComponentLocation());
		Effect->SetVariableVec3(TEXT("EndLocation"), OtherNode->GetComponentLocation());
		return Effect;
	}
	return nullptr;
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
	for (FBeamConnection Connection : NodesSupplying)
	{
		if (Connection.Child)
		{
			Connection.Child->PowerOff(Iteration + 1);
		}
		if (Connection.Effect)
		{
			Connection.Effect->DestroyComponent();
		}
	}
	NodesSupplying.Empty();
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
			bool Contains = false;
			for (FBeamConnection Connection : NodesSupplying)
			{
				/// Todo: figure out how to properly implement Contains() method.
				if (Connection.Child == Node)
				{
					Contains = true;
					break;
				}
			}
			if (!Contains)
			{
				UBeamNodeComponent* Origin = bSelfPowered ? this : PowerOrigin.Get();
				NodesSupplying.Add(FBeamConnection(Node, SpawnEffectComponent(Node)));
				Node->PowerOn(this, Origin, Iteration);
			}
		}
	}
}


void UBeamNodeComponent::ValidateConnections()
{
	TArray<int> NodesPendingDeletion;
	for (int i = 0; i < NodesSupplying.Num(); i++)
	{
		UBeamNodeComponent* Node = NodesSupplying[i].Child;
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
			if (NodesSupplying[IndexToDelete].Effect)
			{
				NodesSupplying[IndexToDelete].Effect->DestroyComponent();
			}
			NodesSupplying.RemoveAt(IndexToDelete);
		}
	}
}
