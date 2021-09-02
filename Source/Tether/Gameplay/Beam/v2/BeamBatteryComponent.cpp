// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "BeamBatteryComponent.h"


FEnergyRequest::FEnergyRequest()
	:RequestAmount(0.0f), Requester(nullptr)
{}


FEnergyRequest::FEnergyRequest(const float RequestAmount, UBeamReceiverComponent* Requester)
	:RequestAmount(RequestAmount), Requester(Requester)
{}



// Sets default values for this component's properties
UBeamBatteryComponent::UBeamBatteryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bActiveWhenUnpowered = true;
}


void UBeamBatteryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	bSelfPowered = Energy >= 0.0f;
	PowerReceivers(DeltaTime);
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UBeamBatteryComponent::ReceiveEnergyRequest(const float RequestAmount, UBeamReceiverComponent* Requester)
{
	Requests.Add(FEnergyRequest(RequestAmount, Requester));
	RequestedEnergy += RequestAmount;
}


float UBeamBatteryComponent::GetStoragePercent() const
{
	if (MaxEnergy >= 0.0f && Energy >= 0.0f)
	{
		return Energy / MaxEnergy;
	}
	return 0.0f;
}


void UBeamBatteryComponent::PowerReceivers(const float DeltaTime)
{
	// How much energy we have gained or lost this tick
	const float EnergyDelta = (GenerationRate - RequestedEnergy) * DeltaTime;

	float PowerRatio = 1.0f;

	if (EnergyDelta < 0.0f)
	{
		if (Energy <= 0.0f)
		{
			PowerRatio = RequestedEnergy == 0.0f ? 0.0f : GenerationRate / RequestedEnergy;
		}
	}
	Energy = FMath::Clamp(Energy + EnergyDelta, 0.0f, MaxEnergy);
	TArray<int> PendingDeletionIndices;
	for (int i = 0; i < Requests.Num(); i++)
	{
		if (!Requests[i].Requester.IsValid() || Requests[i].Requester->GetOrigin() != this)
		{
			PendingDeletionIndices.Insert(i, 0);
		}
		else
		{
			Requests[i].Requester->RecieveEnergy(PowerRatio);
		}
	}
	// Delete the indices in reverse order...
	for (int PendingDeletionIndex : PendingDeletionIndices)
	{
		RequestedEnergy -= Requests[PendingDeletionIndex].RequestAmount;
		Requests.RemoveAt(PendingDeletionIndex);
	}
}

