// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "LinearMovementComponent.h"

#include "Tether/Tether.h"
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
	
}

void ULinearMovementComponent::Move(const float DeltaTime)
{
	AActor* Parent = GetOwner();
	UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Parent->GetComponentByClass(UPrimitiveComponent::StaticClass()));
	const FVector NewLocation = PrimitiveComponent->GetComponentLocation() + Direction.RotateVector(Velocity) * DeltaTime;
	TArray<FHitResult> Results;

	const bool bBlockingHitFound = GetWorld()->SweepMultiByProfile(Results, PrimitiveComponent->GetComponentLocation(), NewLocation, PrimitiveComponent->GetComponentQuat(), PrimitiveComponent->GetCollisionProfileName(), PrimitiveComponent->GetCollisionShape());
	if (bBlockingHitFound)
	{
		for (FHitResult HitResult : Results)
		{
			if (ATetherCharacter* Character = Cast<ATetherCharacter>(HitResult.GetActor()))
			{
				// Character->AddActorWorldOffset( (1 - HitResult.Time) * DeltaTime * Velocity );
				const float SelfVelocityAlongNormal = FVector::DotProduct(Velocity, -HitResult.Normal);
				const float TargetVelocityAlongNormal = FVector::DotProduct(Character->GetVelocity(), -HitResult.Normal);
				if (SelfVelocityAlongNormal > TargetVelocityAlongNormal)
				{
					const FVector Impulse = (SelfVelocityAlongNormal - TargetVelocityAlongNormal) * -HitResult.Normal;
					// UE_LOG(LogTetherGame, Display, TEXT("Impulse added to character %s"), *Impulse.ToString());
					Character->MovementComponent->AddImpulse(Velocity);
				}
			}
		}
	}
	Parent->SetActorLocation(NewLocation, false);
}


// Called every frame
void ULinearMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Move(DeltaTime);
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void ULinearMovementComponent::SetVelocity(const FVector& InVelocity)
{
	Velocity = InVelocity;
}


void ULinearMovementComponent::SetVelocity(const float& InVelocity)
{
	Velocity = FVector(InVelocity, 0, 0);
}


