// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "TailComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UTailComponent::UTailComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTailComponent::BeginPlay()
{
	Super::BeginPlay();

	Offset = GetRelativeLocation();
	SetUsingAbsoluteLocation(true);
}


// Called every frame
void UTailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner())
	{
		SetWorldLocation(StoreLocation(GetOwner()->GetActorLocation() + GetOwner()->GetActorRotation().RotateVector(Offset)));
	}
}


FVector UTailComponent::StoreLocation(const FVector& Location)
{
	FVector OldestLocation;
	if (QueueCount < QueueSize)
	{
		PreviousLocations.Enqueue(Location);
		PreviousLocations.Peek(OldestLocation);
		QueueCount++;
	}
	else
	{
		PreviousLocations.Dequeue(OldestLocation);
		PreviousLocations.Enqueue(Location);
	}
	return OldestLocation;
}

