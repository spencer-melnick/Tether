// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "BeamReceiverComponent.h"

#include "BeamBatteryComponent.h"


UBeamReceiverComponent::UBeamReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bActiveWhenUnpowered = true;
}

// Called every frame
void UBeamReceiverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	ValidateConnections();
	if (UBeamBatteryComponent* Battery = Cast<UBeamBatteryComponent>(PowerOrigin))
	{
		const float EnergyConsumed = Battery->ConsumeEnergy(EnergyConsumptionRate * DeltaTime);
		bActive = EnergyConsumed >= EnergyConsumptionRate * DeltaTime;
	}
	else
	{
		bActive = false;
	}
}

