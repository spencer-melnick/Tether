// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "LinearMovementComponent.h"

// Sets default values for this component's properties
ULinearMovementComponent::ULinearMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}


// Called when the game starts
void ULinearMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	Direction = GetOwner()->GetActorRotation();
	
}


// Called every frame
void ULinearMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	AActor* Parent = GetOwner();
	const FVector NewLocation = Parent->GetActorLocation() + Direction.RotateVector(LinearVelocity) * DeltaTime;
	Parent->SetActorLocation(NewLocation);
}

void ULinearMovementComponent::SetVelocity(const FVector& Velocity)
{
	LinearVelocity = Velocity;
}

void ULinearMovementComponent::SetVelocity(const float& Velocity)
{
	LinearVelocity = FVector(Velocity, 0, 0);
}


