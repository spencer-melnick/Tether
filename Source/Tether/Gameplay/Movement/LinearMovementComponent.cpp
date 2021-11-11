// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "LinearMovementComponent.h"

#include "Tether/Character/TetherCharacter.h"

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

	SetTickGroup(ETickingGroup::TG_DuringPhysics);
	Direction = GetOwner()->GetActorRotation();

	AActor* Parent = GetOwner();
	for (UActorComponent* Component : Parent->GetComponents())
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
		{
			if (PrimitiveComponent->GetCollisionProfileName() != TEXT("NoCollision"))
			{
				UpdatedComponents.Add(PrimitiveComponent);
			}
		}
	}
}

void ULinearMovementComponent::Move(const float DeltaTime) const
{
	AActor* Parent = GetOwner();
	if (!Parent)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		for (UPrimitiveComponent* PrimitiveComponent : UpdatedComponents)
		{
			if (!IsValid(PrimitiveComponent))
			{
				continue;
			}
			const FVector NewLocation = PrimitiveComponent->GetComponentLocation() + Direction.RotateVector(Velocity) * DeltaTime;
			TArray<FHitResult> Results;

			const bool bBlockingHitFound = World->SweepMultiByProfile(Results, PrimitiveComponent->GetComponentLocation(), NewLocation, PrimitiveComponent->GetComponentQuat(), PrimitiveComponent->GetCollisionProfileName(), PrimitiveComponent->GetCollisionShape());
			if (bBlockingHitFound)
			{
				for (FHitResult HitResult : Results)
				{
					if (ATetherCharacter* Character = Cast<ATetherCharacter>(HitResult.GetActor()))
					{
						Character->MovementComponent->Push(HitResult, PrimitiveComponent->GetComponentRotation().RotateVector(Velocity), PrimitiveComponent);
					}
				}
			}
			PrimitiveComponent->SetWorldLocation(NewLocation, false);
		}
	}
}


// Called every frame
void ULinearMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Move(DeltaTime);
	// Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void ULinearMovementComponent::SetVelocity(const FVector& InVelocity)
{
	Velocity = InVelocity;
}


void ULinearMovementComponent::SetVelocity(const float& InVelocity)
{
	Velocity = FVector(InVelocity, 0, 0);
}


#if WITH_EDITOR
void ULinearMovementComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_STRING_CHECKED(ULinearMovementComponent, Velocity))
	{
		AActor* Parent = GetOwner();
		for (UPrimitiveComponent* PrimitiveComponent : UpdatedComponents)
		{
			if (PrimitiveComponent)
			{
				PrimitiveComponent->ComponentVelocity = Velocity;
			}
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif