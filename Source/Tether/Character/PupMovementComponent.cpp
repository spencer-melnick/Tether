// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "PupMovementComponent.h"

#include "DrawDebugHelpers.h"
#include "TetherCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"


namespace PupMovementCVars
{
	static float TimestepLength = 1.f / 60.f;
	static FAutoConsoleVariableRef CVarTimestepLength(
		TEXT("PupMovement.TimestepLength"),
		TimestepLength,
		TEXT("Maximum length of a physics timestep before splitting into multiple steps"),
		ECVF_Default);
	
	static float MinimumTraceDistance = 0.5f;
	static FAutoConsoleVariableRef CVarMinimumTraceDistance(
		TEXT("PupMovement.MinimumTraceDistance"),
		MinimumTraceDistance,
		TEXT("Minimum distance to trace while moving"),
		ECVF_Default);

	static int32 MaxSubsteps = 3;
	static FAutoConsoleVariableRef CVarMaxSubsteps(
		TEXT("PupMovement.MaxSubsteps"),
		MaxSubsteps,
		TEXT("Maximum number of substeps to perform while resolving movement collision"),
		ECVF_Default);
}

UPupMovementComponent::UPupMovementComponent()
{
}

void UPupMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* TickFunction)
{
	// Gather all of the input we've accumulated since the last frame
	HandleInputAxis();
	
	while (DeltaTime > SMALL_NUMBER)
	{
		// Break frame time into actual movement steps
		const float ActualStepLength = FMath::Min(DeltaTime, PupMovementCVars::TimestepLength);
		DeltaTime -= ActualStepLength;

		StepMovement(ActualStepLength);
	}
}



bool UPupMovementComponent::SweepCapsule(const FVector Offset, FHitResult& OutHit) const
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
		QueryParams.bIgnoreTouches = true;
		
		const FCollisionResponseParams ResponseParams = FCollisionResponseParams::DefaultResponseParam;
		
		return World->SweepSingleByChannel(OutHit, Start, End, Capsule->GetComponentQuat(), ECC_Pawn,
			Capsule->GetCollisionShape(), QueryParams, ResponseParams);
	}
	return false;
}

void UPupMovementComponent::SetDefaultMovementMode()
{
	FHitResult FloorResult;
	if (FindFloor(10.f, FloorResult) && Velocity.Z <= 0.0f)
	{
		MovementMode = EPupMovementMode::M_Walking;
		SnapToFloor(FloorResult);
	}
	else
	{
		MovementMode = EPupMovementMode::M_Falling;
	}
}


void UPupMovementComponent::AddImpulse(const FVector Impulse)
{
	PendingImpulses += Impulse;
}


float UPupMovementComponent::GetMaxSpeed() const
{
	return MaxSpeed;
}


bool UPupMovementComponent::FindFloor(const float Distance, FVector& Location)
{
	FHitResult FloorHitResult;
	const FVector Offset = FVector(0.0f, 0.0f, FMath::Min(Distance, 0.0f));
	if (SweepCapsule(Offset, FloorHitResult))
	{
		UPrimitiveComponent* FloorComponent = FloorHitResult.GetComponent();
		CurrentFloorComponent = FloorComponent;
		Location = FloorHitResult.Location;

		if (FloorHitResult.bStartPenetrating)
		{
			return false;
		}
		
		RenderHitResult(FloorHitResult, FColor::Red);
		return FloorComponent && FloorComponent->CanCharacterStepUp(GetPawnOwner());
	}
	CurrentFloorComponent = nullptr;
	return false;
}

bool UPupMovementComponent::FindFloor(float SweepDistance, FHitResult& OutHitResult) const
{
	const FVector SweepOffset = FVector::DownVector * SweepDistance;
	if (SweepCapsule(SweepOffset, OutHitResult))
	{
		if (IsValidFloorHit(OutHitResult))
		{
			return true;
		}

		// If this wasn't a valid floor hit, clear the hit result but keep the trace data
		OutHitResult.Reset(1.f, true);
	}

	return false;
}

bool UPupMovementComponent::IsValidFloorHit(const FHitResult& FloorHit) const
{
	// Check if we actually hit a floor component
	const UPrimitiveComponent* FloorComponent = FloorHit.GetComponent();
	if (FloorComponent && FloorComponent->CanCharacterStepUp(GetPawnOwner()))
	{
		// Optionally check floor slope here!
		return true;
	}

	return false;
}

void UPupMovementComponent::SnapToFloor(const FHitResult& FloorHit)
{
	FHitResult DiscardHit;
	SafeMoveUpdatedComponent(FloorHit.Location - UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentQuat(), true, DiscardHit);
}

bool UPupMovementComponent::Jump()
{
	if (bCanJump)
	{
		AddImpulse(UpdatedComponent->GetUpVector() * 500.0f);
		bCanJump = false;
		return true;
	}
	return false;
}

float UPupMovementComponent::GetGravityZ() const
{
	// Gravity scale here?
	return Super::GetGravityZ();
}

void UPupMovementComponent::AnchorToLocation(const FVector& AnchorLocationIn)
{
	AnchorLocation = AnchorLocationIn;
	MovementMode = EPupMovementMode::M_Anchored;
	const FVector Offset = AnchorLocationIn - UpdatedComponent->GetComponentLocation();
	DesiredRotation = FRotator(0.0f, FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X)), 0.0f);
}

void UPupMovementComponent::BreakAnchor(const bool bForceBreak)
{
	AnchorLocation = FVector::ZeroVector;
	if (bForceBreak)
	{
		MovementMode = EPupMovementMode::M_Deflected;
	}
	else
	{
		SetDefaultMovementMode();
	}
}


void UPupMovementComponent::StepMovement(float DeltaTime)
{
	// Update rotation
	const FRotator NewRotation = GetNewRotation(DeltaTime);
	if (UpdatedComponent)
	{
		UpdatedComponent->SetWorldRotation(NewRotation);
	}
	Velocity = GetNewVelocity(DeltaTime);

	
	if (MovementMode == EPupMovementMode::M_Walking ||  MovementMode == EPupMovementMode::M_Falling || MovementMode == EPupMovementMode::M_Deflected)
	{
		// First check if we're on the floor
		FHitResult FloorHit;

		if (FindFloor(10.f, FloorHit) && Velocity.Z <= 0.0f)
		{
			if (!bGrounded && MovementMode == EPupMovementMode::M_Falling)
			{
				Land();
			}
			bGrounded = true;
			Velocity.Z = 0.f;
			SnapToFloor(FloorHit);
		}
		else
		{
			if(bGrounded && MovementMode == EPupMovementMode::M_Walking)
			{
				Fall();
			}
			bGrounded = false;
			// To avoid potential stuttering, only apply gravity if we're not on the ground
			Velocity.Z += GetGravityZ() * DeltaTime;
		}
	}

	// Break up physics into substeps based on collisions
	int32 NumSubsteps = 0;
	while (NumSubsteps < PupMovementCVars::MaxSubsteps && DeltaTime > SMALL_NUMBER)
	{
		NumSubsteps++;
		DeltaTime -= SubstepMovement(DeltaTime);
	}

	// Update the alpha value to be used for turning friction
	MovementSpeedAlpha = Velocity.IsNearlyZero() ? 0.0f : Velocity.Size2D() / MaxSpeed;
	
	// Let our primitive component know what its new velocity should be
	UpdateComponentVelocity();
}

float UPupMovementComponent::SubstepMovement(float DeltaTime)
{
	FHitResult HitResult;
	const FVector Movement = Velocity * DeltaTime;
	
	SafeMoveUpdatedComponent(Movement, UpdatedComponent->GetComponentQuat(), true, HitResult);
	
	if (HitResult.bBlockingHit)
	{
		const float ImpactVelocityMagnitude = FMath::Max(FVector::DotProduct(-HitResult.Normal, Velocity), 0.f);
		Velocity += HitResult.Normal * ImpactVelocityMagnitude;

		return HitResult.Time * DeltaTime;
	}

	// Completed the move with no collisions!
	return DeltaTime;
}

void UPupMovementComponent::Land()
{
	MovementMode = EPupMovementMode::M_Walking;
	bCanJump = true;
}

void UPupMovementComponent::Fall()
{
	MovementMode = EPupMovementMode::M_Falling;
	if(UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CoyoteTimer, FTimerDelegate::CreateWeakLambda(this, [this]
		{
			if(!bGrounded)
			{
				bCanJump = false;
			}
		}), CoyoteTime, false);
	}
}


void UPupMovementComponent::TickGravity(const float DeltaTime)
{
	if (UWorld* World = GetWorld())
	{
		Velocity.Z = FMath::Max(Velocity.Z + World->GetGravityZ() * DeltaTime, TerminalVelocity);
	}
}


void UPupMovementComponent::HandleInputAxis()
{
	const FVector InputVector = ConsumeInputVector();
	if (MovementMode == EPupMovementMode::M_Walking || MovementMode == EPupMovementMode::M_Falling || MovementMode == EPupMovementMode::M_Deflected)
	{
		if (!InputVector.IsNearlyZero())
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
}


FRotator UPupMovementComponent::GetNewRotation(const float DeltaTime) const
{
	switch (MovementMode)
	{
	case EPupMovementMode::M_Walking:
		{
			float RotationRate = RotationSpeed;
			if (bSlip)
			{
				RotationRate *= 1 - (1 - SlipFactor) * MovementSpeedAlpha;
			}
			return FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime, RotationRate);
		}
	case EPupMovementMode::M_Falling:
		{
			const float RotationRate = RotationSpeed * SlipFactor;
			return FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime, RotationRate);
		}
	case EPupMovementMode::M_Anchored:
		{
			return FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime, SnapRotationVelocity);
		}
	default:
		{
			return UpdatedComponent->GetComponentRotation();
		}
	}
}


FVector UPupMovementComponent::GetNewVelocity(const float DeltaTime)
{
	switch (MovementMode)
	{
		case EPupMovementMode::M_Walking:
			{
				const FVector Acceleration = MaxAcceleration * DirectionVector * DeltaTime;
				const FVector NewVelocity = (Velocity + Acceleration).GetClampedToMaxSize2D(MaxSpeed) + ConsumeImpulse();
				return ApplyFriction(NewVelocity, DeltaTime);
			}
		case EPupMovementMode::M_Falling:
			{
				const FVector Acceleration = MaxAcceleration * DirectionVector * DeltaTime * AirControlFactor;
				return (Velocity + Acceleration).GetClampedToMaxSize2D(MaxSpeed) + ConsumeImpulse();
			}
		case EPupMovementMode::M_Anchored:
			{
				const FVector Offset = AnchorLocation - UpdatedComponent->GetComponentLocation();
				FVector NewVelocity = Offset.GetSafeNormal() * SnapVelocity;
				NewVelocity.Z = 0.0f;
				return NewVelocity;				
			}
		case EPupMovementMode::M_Deflected:
			{
				return Velocity + ConsumeImpulse();
			}
		default:
			{
				return FVector::ZeroVector;
			}
	}
}


FVector UPupMovementComponent::ApplyFriction(const FVector& VelocityIn, const float DeltaTime) const
{
	if(bIsWalking)
	{
		const FVector DirectionVectorNormalized = DirectionVector.GetSafeNormal();
		FVector VelocityInCurrentDirection = FVector::DotProduct(VelocityIn, DirectionVectorNormalized) * DirectionVectorNormalized;
		VelocityInCurrentDirection.Z += VelocityIn.Z;
		return FMath::VInterpConstantTo(VelocityIn, VelocityInCurrentDirection, DeltaTime, RunningFriction);
	}
	const FVector NewVelocity = FMath::VInterpConstantTo(VelocityIn, FVector::ZeroVector, DeltaTime, BreakingFriction);
	return FVector(NewVelocity.X, NewVelocity.Y, VelocityIn.Z);
}


void UPupMovementComponent::RenderHitResult(const FHitResult& HitResult, const FColor Color) const
{
	if (HitResult.GetComponent())
	{
		DrawDebugLine(GetWorld(), HitResult.ImpactPoint, HitResult.ImpactPoint + HitResult.ImpactNormal * 100.0f, Color, false, 1.0f, 1.0f, 2.0f);
		DrawDebugString(GetWorld(), HitResult.ImpactPoint + HitResult.ImpactNormal * 100.0f, HitResult.GetComponent()->GetName(), nullptr, Color, 0.02f, false, 1.0f);
	}
}

FVector UPupMovementComponent::ConsumeImpulse()
{
	const FVector ConsumedVector = PendingImpulses;
	PendingImpulses = FVector::ZeroVector;
	return ConsumedVector;
}
