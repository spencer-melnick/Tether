// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "Conveyor.h"


UConveyorComponent::UConveyorComponent()
{
}

void UConveyorComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
}

void UConveyorComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

// Called when the game starts or when spawned
void UConveyorComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

FVector UConveyorComponent::GetAppliedVelocity() const
{
	return GetComponentRotation().RotateVector(BeltVelocity);
}

