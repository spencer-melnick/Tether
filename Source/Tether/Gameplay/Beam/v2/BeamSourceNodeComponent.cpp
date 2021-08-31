// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "BeamSourceNodeComponent.h"


// Sets default values for this component's properties
UBeamSourceNodeComponent::UBeamSourceNodeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	bSelfPowered = true;
	bRecieveConnections = false;
}