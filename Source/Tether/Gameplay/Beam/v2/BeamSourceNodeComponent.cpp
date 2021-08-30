// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "BeamSourceNodeComponent.h"


// Sets default values for this component's properties
UBeamSourceNodeComponent::UBeamSourceNodeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


void UBeamSourceNodeComponent::BeginPlay()
{
	Super::BeginPlay();
	bPowered = true;
	PowerOrigin = this;
}


void UBeamSourceNodeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	TryPowerNodes();
	ValidateConnections();
}

