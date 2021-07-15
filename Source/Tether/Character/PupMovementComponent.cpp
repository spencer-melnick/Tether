// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "PupMovementComponent.h"

#include "TetherCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UPupMovementComponent::UPupMovementComponent()
{

}

void UPupMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* TickFunction)
{
	HandleInputAxis();
	UpdateGravity(DeltaTime);
	
	UpdatedComponent->SetWorldRotation(GetNewRotation(DeltaTime));
	if(bIsWalking)
	{
		Velocity = GetNewVelocity(DeltaTime);
	}

	ApplyFriction(DeltaTime);
	UpdateComponentVelocity();
	UpdatedComponent->AddWorldOffset(Velocity * DeltaTime);
}



bool UPupMovementComponent::SweepCapsule(const FVector Offset, FHitResult& OutHit, bool bIgnoreFloor) const
{
	ATetherCharacter* Character = Cast<ATetherCharacter>(GetPawnOwner());
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	UWorld* World = GetWorld();
	
	if (IsValid(Character) && IsValid(Capsule) && World)
	{
		const FVector Start = Capsule->GetComponentLocation();
		const FVector End = Start + Offset;
		
		FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
		QueryParams.AddIgnoredActor(Character);

		const FCollisionResponseParams ResponseParams = FCollisionResponseParams::DefaultResponseParam;
		
		return World->SweepSingleByChannel(OutHit, Start, End, Capsule->GetComponentQuat(), ECC_Pawn,
			Capsule->GetCollisionShape(), QueryParams, ResponseParams);
	}
	return false;
}


void UPupMovementComponent::AddImpulse(const FVector Impulse)
{
	Velocity += Impulse;
}


float UPupMovementComponent::GetMaxSpeed() const
{
	return MaxSpeed;
}


bool UPupMovementComponent::CheckForFloor(const float Distance, FVector& Location)
{
	FHitResult FloorHitResult;
	if (SweepCapsule(FVector(0.0f, 0.0f, Distance), FloorHitResult))
	{
		UPrimitiveComponent* FloorComponent = FloorHitResult.GetComponent();
		Location = FloorHitResult.Location;

		if (FloorHitResult.bStartPenetrating)
		{
			Location = UpdatedComponent->GetComponentLocation() + FloorHitResult.Normal * FloorHitResult.PenetrationDepth;
		}
		return FloorComponent && FloorComponent->CanCharacterStepUp(GetPawnOwner());
	}

	return false;
}


void UPupMovementComponent::UpdateGravity(float DeltaTime)
{
	FVector NewLocation;
	const float FloorDistance = FMath::Min (DeltaTime * Velocity.Z, -0.f);
	if (CheckForFloor(FloorDistance, NewLocation))
	{
		Velocity.Z = FMath::Max(Velocity.Z, 0.0f);
		UpdatedComponent->SetWorldLocation(NewLocation);
		bGrounded = true;
	}
	else
	{
		// We are falling!
		bGrounded = false;
		if (UWorld* World = GetWorld())
		{
			Velocity.Z = FMath::Max(Velocity.Z + World->GetGravityZ() * DeltaTime, TerminalVelocity);
		}
	}
}

void UPupMovementComponent::HandleInputAxis()
{
	const FVector InputVector = ConsumeInputVector();
	if(InputVector != FVector::ZeroVector)
	{
		bIsWalking = true;
		DirectionVector = UpdatedComponent->GetForwardVector().RotateAngleAxis(CameraYaw, UpdatedComponent->GetUpVector()) * FMath::Min(InputVector.Size(), 1.0f);
		const float Angle = FMath::RadiansToDegrees(FMath::Atan2(InputVector.GetSafeNormal2D().Y, InputVector.GetSafeNormal2D().X));
		DesiredRotation.Yaw = Angle + CameraYaw;
	}
	else
	{
		bIsWalking = false;
		DirectionVector = FVector::ZeroVector;
	}
	
}


FRotator UPupMovementComponent::GetNewRotation(float DeltaTime)
{
	float RotationRate = RotationSpeed;
	if (bSlip)
	{
		RotationRate *= 1 - (1 - SlipFactor) * (Velocity.Size2D() / MaxSpeed);
	}
	return FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime, RotationRate);
}


FVector UPupMovementComponent::GetNewVelocity(float DeltaTime)
{
	FVector Acceleration = MaxAcceleration * DirectionVector * DeltaTime;
	if (!bGrounded)
	{
		Acceleration *= AirControlFactor;
	}
	return (Velocity + Acceleration).GetClampedToMaxSize2D(MaxSpeed);
}


void UPupMovementComponent::ApplyFriction(float DeltaTime)
{
	if(bGrounded)
	{
		if(bIsWalking)
		{
			const FVector DirectionVectorNormalized = DirectionVector.GetSafeNormal();
			FVector VelocityInCurrentDirection = FVector::DotProduct(Velocity, DirectionVectorNormalized) * DirectionVectorNormalized;
			VelocityInCurrentDirection.Z += Velocity.Z;
			Velocity = FMath::VInterpConstantTo(Velocity, VelocityInCurrentDirection, DeltaTime, RunningFriction);
		}
		else
		{
			const FVector NewVelocity = FMath::VInterpConstantTo(Velocity, FVector::ZeroVector, DeltaTime, BreakingFriction);
			Velocity.X = NewVelocity.X;
			Velocity.Y = NewVelocity.Y;
		}
	}
}