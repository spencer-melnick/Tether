// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "BeamBatteryComponent.h"


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
	if (GenerationRate >= 0.0f)
	{
		Energy = FMath::Min(Energy + GenerationRate * DeltaTime, MaxEnergy);
	}
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


float UBeamBatteryComponent::ConsumeEnergy(const float EnergyConsumed)
{
	float EnergyProvided = EnergyConsumed;
	if (Energy <= EnergyConsumed)
	{
		EnergyProvided = Energy;
		Energy = 0.0f;
	}
	else
	{
		Energy -= EnergyConsumed;
	}
	return EnergyProvided;
}


float UBeamBatteryComponent::GetStoragePercent() const
{
	if (MaxEnergy >= 0.0f && Energy >= 0.0f)
	{
		return Energy / MaxEnergy;
	}
	return 0.0f;
}

