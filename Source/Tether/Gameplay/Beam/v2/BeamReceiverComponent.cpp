// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "BeamReceiverComponent.h"

#include "BeamBatteryComponent.h"


UBeamReceiverComponent::UBeamReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bActiveWhenUnpowered = true;
	bSendConnections = false;
}

// Called every frame
void UBeamReceiverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	bActive = PowerPercentage > 0.0f;
}

void UBeamReceiverComponent::PowerOn(UBeamNodeComponent* Source, UBeamNodeComponent* Origin, int Iteration)
{
	Super::PowerOn(Source, Origin, Iteration);
	if (UBeamBatteryComponent* Battery = Cast<UBeamBatteryComponent>(Origin))
	{
		Battery->ReceiveEnergyRequest(EnergyConsumptionRate, this);
	}
}


void UBeamReceiverComponent::PowerOff(int Iteration)
{
	PowerPercentage = 0.0f;
	Super::PowerOff(Iteration);
}


void UBeamReceiverComponent::RecieveEnergy(const float EnergyRatio)
{
	PowerPercentage = EnergyRatio;
}


float UBeamReceiverComponent::GetPowerPercentage() const
{
	return PowerPercentage;
}

