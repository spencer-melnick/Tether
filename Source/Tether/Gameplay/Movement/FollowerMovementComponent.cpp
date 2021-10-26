// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "FollowerMovementComponent.h"

#include "Tether/Character/TetherCharacter.h"

void UFollowerMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                               FActorComponentTickFunction* ThisTickFunction)
{
	if (Target.IsValid())
	{
		const FVector TargetLocation = Target->GetActorLocation();
		const FVector Location = UpdatedComponent->GetComponentLocation();

		const FVector Distance = TargetLocation - Location;
		const FRotator DesiredRotation = Distance.Rotation();

		const FRotator NewRotation = FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime, RotationSpeed);
		UpdatedComponent->SetWorldRotation(NewRotation);

		Velocity = UpdatedComponent->GetForwardVector() * ScalarVelocity;
		UpdateComponentVelocity();
		const FVector Movement = Velocity * DeltaTime;

		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(GetOwner()->GetComponentByClass(UPrimitiveComponent::StaticClass())))
		{
			TArray<FHitResult> HitResults;
			GetWorld()->SweepMultiByProfile(HitResults, Location, Location + Movement, NewRotation.Quaternion(), PrimitiveComponent->GetCollisionProfileName(), PrimitiveComponent->GetCollisionShape());
			for (FHitResult HitResult : HitResults)
			{
				PrimitiveComponent->DispatchBlockingHit(*GetOwner(), HitResult);
				/* if (ATetherCharacter* Character = Cast<ATetherCharacter>(HitResult.GetActor()))
				{
					Character->DeflectSimple(HitResult.Normal * -500.0f, 1.0f);
				} */
			}
		}
		UpdatedComponent->AddWorldOffset(Movement);
	}
}
