// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "TetherCharacterMovementComponent.h"

#include "PhysXInterfaceWrapperCore.h"
#include "GameFramework/Character.h"

void UTetherCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch(CustomMovementMode)
	{
		case ETetherMovementType::Anchored:
			UpdatedComponent->AddLocalRotation(GetAnchorDeltaRotation(deltaTime));
			UpdatedComponent->SetWorldLocation(GetAnchorDeltaLocation(deltaTime));
		break;
		
		default:

		break;
	}
}

void UTetherCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode,
	uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if(MovementMode == MOVE_Custom && CustomMovementMode == static_cast<int>(ETetherMovementType::Anchored))
	{
		ClearAccumulatedForces();
		Velocity = FVector::ZeroVector;
	}
}

void UTetherCharacterMovementComponent::SetAnchorRotation(const FRotator Rotation)
{
		AnchorRotation = Rotation;
}

void UTetherCharacterMovementComponent::SetAnchorLocation(const FVector Location)
{
	AnchorLocation = Location;
}

FRotator UTetherCharacterMovementComponent::GetAnchorDeltaRotation(float DeltaTime)
{
	FRotator Delta = AnchorRotation - GetOwner()->GetActorRotation();
	Delta.Normalize();
	const float DeltaPitch = FMath::FInterpConstantTo(0, Delta.Pitch, DeltaTime, RotationRate.Pitch);
	const float DeltaYaw = FMath::FInterpConstantTo(0, Delta.Yaw, DeltaTime, RotationRate.Yaw);
	const float DeltaRoll = FMath::FInterpConstantTo(0, Delta.Roll, DeltaTime, RotationRate.Roll);

	return FRotator(DeltaPitch, DeltaYaw, DeltaRoll);

}

FVector UTetherCharacterMovementComponent::GetAnchorDeltaLocation(float DeltaTime)
{
	return FMath::VInterpConstantTo(GetCharacterOwner()->GetActorLocation(), AnchorLocation, DeltaTime, MaxCustomMovementSpeed);
}
