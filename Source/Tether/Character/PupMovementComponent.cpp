// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "PupMovementComponent.h"

#include "AITypes.h"
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




void UPupMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	SetDefaultMovementMode();
	LastBasisPosition = UpdatedComponent->GetComponentLocation();
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


void UPupMovementComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.GetPropertyName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UPupMovementComponent, MaxIncline))
	{
		MaxInclineZComponent = FMath::Cos(FMath::DegreesToRadians(MaxIncline));
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UPupMovementComponent, ApexVelocity) || PropertyName ==
		GET_MEMBER_NAME_CHECKED(UPupMovementComponent, MaxJumpTime) || PropertyName == GET_MEMBER_NAME_CHECKED(
			UPupMovementComponent, JumpInitialVelocity))
	{
		HoldJumpAcceleration = MaxJumpTime <= 0.0f ? 0.0f : (ApexVelocity - JumpInitialVelocity) / MaxJumpTime;
	}
}


float UPupMovementComponent::GetMaxSpeed() const
{
	return MaxSpeed;
}


float UPupMovementComponent::GetGravityZ() const
{
	// Gravity scale here?
	return Super::GetGravityZ();
}


bool UPupMovementComponent::ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit,
	const FQuat& NewRotation)
{
	if (MovementMode == EPupMovementMode::M_Walking || MovementMode == EPupMovementMode::M_Falling || MovementMode == EPupMovementMode::M_Deflected)
	{
		if (Hit.GetComponent())
		{
			// We shouldn't copy the velocity from slight floor glitches -- this could lead to the player
			// being counted as being in the air.
			const FVector AdjustmentWithoutBasis = Adjustment - FVector::DotProduct(Adjustment, FloorNormal) * FloorNormal;

			if (AdjustmentWithoutBasis.Size() >= 1.0f)
			{
				// The further in the sweep the move had to occur, the faster we had to move out of the way
				// AddAdjustment(AdjustmentWithoutBasis / (1 - Hit.Time));
				DrawDebugDirectionalArrow(GetWorld(), UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentLocation() + AdjustmentWithoutBasis * 10.0f, 10.0f, FColor::Red, false, 20.0f, -1);
			}
			
			RenderHitResult(Hit, FColor::Green);
			IgnoredComponentsForSweep.Add(Hit.GetComponent());
		}
	}
	return Super::ResolvePenetrationImpl(Adjustment, Hit, NewRotation);
}


void UPupMovementComponent::StepMovement(const float DeltaTime)
{
	MagnetToBasis(1.0f, DeltaTime);
	if (Velocity == FVector::ZeroVector)
	{
		HandleExternalOverlaps(DeltaTime);
	}

	// Update rotation
	const FRotator NewRotation = GetNewRotation(DeltaTime);
	if (UpdatedComponent)
	{
		const float DeltaYaw = FMath::FindDeltaAngleDegrees(NewRotation.Yaw, UpdatedComponent->GetComponentRotation().Yaw);
		TurningDirection = DeltaYaw;
		UpdatedComponent->SetWorldRotation(NewRotation);
	}
	Velocity = GetNewVelocity(DeltaTime);

	if (MovementMode == EPupMovementMode::M_Walking || MovementMode == EPupMovementMode::M_Falling || MovementMode == EPupMovementMode::M_Deflected)
	{
		// First check if we're on the floor
		FHitResult FloorHit;
		if (FindFloor(1000.0f, FloorHit, 2))
		{
			SnapToFloor(FloorHit);
			if ( FVector::DotProduct(Velocity, FloorNormal) <= 100.0f ) // Avoid floating point errors..
			{
				if (!bGrounded && MovementMode == EPupMovementMode::M_Falling)
				{
					Land();
				}
				bGrounded = true;
				Velocity -= FVector::DotProduct(Velocity, FloorNormal) * FloorNormal;
				// SnapToFloor(FloorHit);
			}
			else
			{
				RenderHitResult(FloorHit, FColor::Purple);
			}
		}
		else
		{
			if (bGrounded && MovementMode == EPupMovementMode::M_Walking)
			{
				Fall();
			}
			bGrounded = false;

			// To avoid potential stuttering, only apply gravity if we're not on the ground
			Velocity.Z += GetGravityZ() * DeltaTime;
		}
		
		if (MovementMode == EPupMovementMode::M_Deflected)
		{
			if (FVector::DotProduct(DeflectDirection, Velocity) <= KINDA_SMALL_NUMBER)
			{
				RegainControl();
			}
		}
	}
	if (MovementMode == EPupMovementMode::M_Recover && bIgnoreObstaclesWhenRecovering)
	{
		UpdatedComponent->SetWorldLocation(UpdatedComponent->GetComponentLocation() + Velocity * DeltaTime, false);
	}
	else
	{
		Velocity = Velocity.GetClampedToMaxSize(TerminalVelocity);
		float NewDeltaTime = DeltaTime;
		// Break up physics into substeps based on collisions
		int32 NumSubsteps = 0;
		while (NumSubsteps < PupMovementCVars::MaxSubsteps && DeltaTime > SMALL_NUMBER)
		{
			NumSubsteps++;
			NewDeltaTime -= SubstepMovement(NewDeltaTime);
		}
	}

	const float KillHeight = -100.0f;
	if (UpdatedComponent->GetComponentLocation().Z <= KillHeight && MovementMode != EPupMovementMode::M_Recover)
	{
		Recover();
	}

	// Update the alpha value to be used for turning friction
	MovementSpeedAlpha = Velocity.IsNearlyZero() ? 0.0f : Velocity.Size2D() / MaxSpeed;

	IgnoredComponentsForSweep.Empty();
	
	// Let our primitive component know what its new velocity should be
	UpdateComponentVelocity();
	StoreBasisTransformPostUpdate();
}


float UPupMovementComponent::SubstepMovement(const float DeltaTime)
{
	FHitResult HitResult;
	const FVector Movement = Velocity * DeltaTime;
	SafeMoveUpdatedComponent(Movement, UpdatedComponent->GetComponentQuat(), true, HitResult);

	// Remember any penetration adjustments we had to do, and push the player next frame
	// Convert our adjustment into a new impulse, based on the DeltaTime that we had when it occurred
	const FVector AdjustmentVelocity = ConsumeAdjustments() / DeltaTime;

	if (!AdjustmentVelocity.IsNearlyZero())
	{
		AddImpulse(AdjustmentVelocity / 2);
	}
	
	if (HitResult.bBlockingHit)
	{
		FVector RelativeVelocity = Velocity;
		if (HitResult.GetComponent())
		{
			RelativeVelocity -= HitResult.GetComponent()->GetComponentVelocity();
		}
		const float ImpactVelocityMagnitude = FMath::Max(FVector::DotProduct(-HitResult.Normal, RelativeVelocity), 0.f);
		Velocity += HitResult.Normal * ImpactVelocityMagnitude;

		return HitResult.Time * DeltaTime;
	}

	// Completed the move with no collisions!
	return DeltaTime;
}


bool UPupMovementComponent::SetMovementMode(const EPupMovementMode& NewMovementMode)
{
	switch (NewMovementMode)
	{
	case EPupMovementMode::M_Anchored:
		{
			bCanJump = false;
			break;
		}
	case EPupMovementMode::M_Deflected:
		{
			if (!bCanJumpWhileDeflected)
			{
				bCanJump = false;
			}
			break;
		}
	case EPupMovementMode::M_Falling:
		{
			if (MovementMode != EPupMovementMode::M_Anchored)
			{
				// If we detached from a floor platform, add the basis velocity to the player, because our reference frame has changed.
				AddImpulse(BasisRelativeVelocity);
			}
			break;
		}
	default:
		break;
	}
	MovementMode = NewMovementMode;
	return true;
}


EPupMovementMode UPupMovementComponent::GetMovementMode() const
{
	return MovementMode;
}


void UPupMovementComponent::SetDefaultMovementMode()
{
	FHitResult FloorResult;
	if ((FindFloor(10.f, FloorResult, 1) && Velocity.Z <= 0.0f) || bGrounded)
	{
		SetMovementMode(EPupMovementMode::M_Walking);
		SnapToFloor(FloorResult);
		bCanJump = true;
	}
	else
	{
		SetMovementMode(EPupMovementMode::M_Falling);
	}
}

void UPupMovementComponent::AnchorToComponent(UPrimitiveComponent* AnchorTargetComponent)
{
	if (!AnchorTargetComponent)
	{
		return;
	}
	
	FHitResult AnchorTestHit;
	const FVector Offset = AnchorTargetComponent->GetComponentLocation() - UpdatedComponent->GetComponentLocation();
	SweepCapsule(Offset, AnchorTestHit);

	if (AnchorTargetComponent == AnchorTestHit.GetComponent())
	{
		AnchorTarget = AnchorTargetComponent;
		AnchorWorldLocation = AnchorTestHit.Location;
		const FVector Distance = UpdatedComponent->GetComponentLocation() - AnchorTarget->GetComponentLocation();
		
		AnchorRelativeLocation =  FVector(FVector::DotProduct(Distance, AnchorTarget->GetForwardVector()),
			FVector::DotProduct(Distance, AnchorTarget->GetRightVector()),
			FVector::DotProduct(Distance, AnchorTarget->GetUpVector()));

		SetMovementMode(EPupMovementMode::M_Anchored);
	}
}


void UPupMovementComponent::AddImpulse(const FVector Impulse)
{
	PendingImpulses += Impulse;
	DrawDebugDirectionalArrow(GetWorld(), UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentLocation() + Impulse, 10.0f, FColor::Green, false, 20.0f, -1);

}


bool UPupMovementComponent::FindFloor(const float SweepDistance, FHitResult& OutHitResult, const int NumTries)
{
	const FVector SweepOffset = FVector::DownVector * SweepDistance;
	for (int i = 0; i < NumTries; i++)
	{
		FHitResult IterativeHitResult;
		if (SweepCapsule(SweepOffset, IterativeHitResult, false))
		{
			OutHitResult = IterativeHitResult;
			RenderHitResult(IterativeHitResult);
			if (IsValidFloorHit(IterativeHitResult))
			{
				CurrentFloorComponent = IterativeHitResult.GetComponent();
				FloorNormal = IterativeHitResult.ImpactNormal;
				return true;
			}
			if (OutHitResult.bStartPenetrating)
			{
				ResolvePenetration(GetPenetrationAdjustment(OutHitResult), OutHitResult, UpdatedComponent->GetComponentQuat());
				IgnoredComponentsForSweep.Add(OutHitResult.GetComponent());
			}
		}
	}
	IgnoredComponentsForSweep.Empty();
	// If this wasn't a valid floor hit, clear the hit result but keep the trace data
	OutHitResult.Reset(1.f, true);
	
	return false;
}


bool UPupMovementComponent::IsValidFloorHit(const FHitResult& FloorHit) const
{
	// Check if we actually hit a floor component
	const UPrimitiveComponent* FloorComponent = FloorHit.GetComponent();
	if (FloorComponent && FloorComponent->CanCharacterStepUp(GetPawnOwner()))
	{
		if (FloorHit.ImpactNormal.Z >= MaxInclineZComponent)
		{
			return true;
		}
	}
	return false;
}


void UPupMovementComponent::SnapToFloor(const FHitResult& FloorHit)
{
	FHitResult DiscardHit;
	DrawDebugPoint(GetWorld(), UpdatedComponent->GetComponentLocation(), 5.0f, FColor::Blue, false, 20.0f, -1);
	if (CheckFloorValidWithinRange(MinimumSafeRadius, FloorHit))
	{
		LastValidLocation = FloorHit.Location;
	}
	SafeMoveUpdatedComponent(FloorHit.Location - UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentQuat(), true, DiscardHit, ETeleportType::None);
}


bool UPupMovementComponent::Jump()
{
	if (bCanJump)
	{
		AddImpulse(UpdatedComponent->GetUpVector() * JumpInitialVelocity);
		
		JumpAppliedVelocity += JumpInitialVelocity;
		bCanJump = false;
		bJumping = true;

		SetMovementMode(EPupMovementMode::M_Falling);
		
		if (UWorld* World = GetWorld())
		{
			if (MaxJumpTime > 0.0f)
			{
				World->GetTimerManager().SetTimer(JumpTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
				{
					StopJumping();
				}), MaxJumpTime, false);
			}
			else
			{
				StopJumping();
			}
		}
		return true;
	}
	return false;
}


void UPupMovementComponent::StopJumping()
{
	bJumping = false;
	JumpAppliedVelocity = 0.0f;
}


void UPupMovementComponent::RegainControl()
{
	DeflectDirection = FVector::ZeroVector;
	if (MovementMode == EPupMovementMode::M_Deflected)
	{
		SetDefaultMovementMode();
	}
}


void UPupMovementComponent::Deflect(const FVector& DeflectionVelocity, const float DeflectTime)
{
	// Don't deflect the character if they are recovering or aren't set to a movement mode
	if (MovementMode == EPupMovementMode::M_Recover || MovementMode == EPupMovementMode::M_None)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		SetMovementMode(EPupMovementMode::M_Deflected);
		DeflectDirection = DeflectionVelocity.GetSafeNormal();
		const float DeflectTimeRemaining = DeflectTimerHandle.IsValid() ? World->GetTimerManager().GetTimerRemaining(DeflectTimerHandle) : 0.f;
		if (DeflectTimeRemaining < DeflectTime)
		{
			World->GetTimerManager().SetTimer(DeflectTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
			{
				RegainControl();
			}), DeflectTime, false);
		}
	}
	AddImpulse(DeflectionVelocity);
}


void UPupMovementComponent::BreakAnchor(const bool bForceBreak)
{
	// These should be thrown out, since any that occurred during the move are invalid
	ConsumeAdjustments();
	if (bForceBreak)
	{
		MovementMode = EPupMovementMode::M_Deflected;
	}
	else
	{
		SetDefaultMovementMode();
	}
	AnchorRelativeLocation = FVector::ZeroVector;
	AnchorWorldLocation = FVector::ZeroVector;
}


void UPupMovementComponent::Land()
{
	SetMovementMode(EPupMovementMode::M_Walking);
	bCanJump = true;
}


void UPupMovementComponent::Fall()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, TEXT("Player fell."));
	SetMovementMode(EPupMovementMode::M_Falling);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CoyoteTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
		{
			if (!bGrounded)
			{
				bCanJump = false;
			}
		}), CoyoteTime, false);
	}
}


void UPupMovementComponent::Recover()
{
	SetMovementMode(EPupMovementMode::M_Recover);
	ClearImpulse();

	if (AActor* Actor = UpdatedComponent->GetOwner())
	{
		const float FallDamage = 20.0f;
		const FDamageEvent DamageEvent;
		Actor->TakeDamage(FallDamage, DamageEvent, Actor->GetInstigatorController(), Actor);
	}
	const FVector RecoveryLocation = LastValidLocation + (RecoveryLevitationHeight * UpdatedComponent->GetUpVector());
	const float GravityDelta = GetGravityZ() * RecoveryTime;
	FVector RecoveryVelocity = (RecoveryLocation - UpdatedComponent->GetComponentLocation()) / RecoveryTime;
	RecoveryVelocity -= (GravityDelta / 2) * UpdatedComponent->GetUpVector();
	Velocity = RecoveryVelocity;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RecoveryTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
		{
			EndRecovery();
		}), RecoveryTime, false);
	}
}


void UPupMovementComponent::EndRecovery()
{
	Velocity.X = 0.0f;
	Velocity.Y = 0.0f;
	UpdatedComponent->SetWorldLocation(LastValidLocation + RecoveryLevitationHeight * UpdatedComponent->GetUpVector());
	LastBasisPosition = UpdatedComponent->GetComponentLocation();
	ClearImpulse();
	SetDefaultMovementMode();
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
	if (APawn* Pawn = GetPawnOwner())
	{
		if (AController* Controller = Pawn->GetController())
		{
			FVector ViewLocation;
			FRotator ViewRotation;
			Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
			CameraYaw = ViewRotation.Yaw;
		}
	}
	const FVector InputVector = ConsumeInputVector();
	if (MovementMode == EPupMovementMode::M_Walking || MovementMode == EPupMovementMode::M_Falling || MovementMode ==
		EPupMovementMode::M_Deflected)
	{
		if (!InputVector.IsNearlyZero())
		{
			bIsWalking = true;
			DirectionVector = UpdatedComponent->GetForwardVector() * FMath::Min(InputVector.Size(), 1.0f);
			const float Angle = FMath::RadiansToDegrees(
				FMath::Atan2(InputVector.GetSafeNormal2D().Y, InputVector.GetSafeNormal2D().X));
			DesiredRotation.Yaw = Angle + CameraYaw;
		}
		else
		{
			bIsWalking = false;
			DirectionVector = FVector::ZeroVector;
			DesiredRotation = UpdatedComponent->GetComponentRotation();
		}
	}
}



// Update method wrappers
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
			return FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime,
			                                RotationRate);
		}
	case EPupMovementMode::M_Falling:
		{
			const float RotationRate = RotationSpeed * SlipFactor;
			return FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime,
			                                RotationRate);
		}
	case EPupMovementMode::M_Anchored:
		{
			return FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime,
			                                SnapRotationVelocity);
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
			DirectionVector = FQuat::FindBetweenNormals(FVector::UpVector, FloorNormal).RotateVector(DirectionVector);
			const FVector Acceleration = DirectionVector.IsNearlyZero()
				                             ? FVector::ZeroVector
				                             : MaxAcceleration * DirectionVector;
			FVector NewVelocity = Velocity + Acceleration * DeltaTime;

			NewVelocity = ClampToPlaneMaxSize(NewVelocity, FloorNormal, MaxSpeed);
			NewVelocity = ApplyFriction(NewVelocity, DeltaTime);
			NewVelocity += ConsumeImpulse();
			return NewVelocity;
		}
	case EPupMovementMode::M_Falling:
		{
			FVector Acceleration = MaxAcceleration * DirectionVector * DeltaTime * AirControlFactor;
			if (bJumping)
			{
				Acceleration += HoldJump(DeltaTime);
			}
			return (Velocity + Acceleration).GetClampedToMaxSize2D(MaxSpeed) + ConsumeImpulse();
		}
	case EPupMovementMode::M_Anchored:
		{
			UpdatedComponent->SetWorldLocation(FMath::VInterpConstantTo(UpdatedComponent->GetComponentLocation(), AnchorWorldLocation, DeltaTime, SnapVelocity));
			return FVector::ZeroVector;
		}
	case EPupMovementMode::M_Deflected:
		{
			FVector NewVelocity = Velocity;
			if (bGrounded)
			{
				NewVelocity = ApplySlidingFriction(Velocity, DeltaTime, DeflectionFriction);
			}
			NewVelocity += DirectionVector.IsNearlyZero()
				               ? FVector::ZeroVector
				               : DeltaTime * MaxAcceleration * DirectionVector * DeflectionControlInfluence;
			return NewVelocity + ConsumeImpulse();
		}
	case EPupMovementMode::M_Recover:
		{
			return Velocity + (GetGravityZ() * DeltaTime * UpdatedComponent->GetUpVector());
		}
	default:
		{
			return FVector::ZeroVector;
		}
	}
}


FVector UPupMovementComponent::HoldJump(const float DeltaTime)
{
	const float JumpAcceleration = FMath::Min(HoldJumpAcceleration * DeltaTime, ApexVelocity - JumpAppliedVelocity);
	JumpAppliedVelocity += JumpAcceleration;
	return JumpAcceleration * UpdatedComponent->GetUpVector();
}


FVector UPupMovementComponent::ApplyFriction(const FVector& VelocityIn, const float DeltaTime) const
{
	if (bIsWalking)
	{
		const FVector VelocityVertical = FVector::DotProduct(FloorNormal, VelocityIn) * FloorNormal;

		// Take the Vertical Velocity (along the Floor Normal) out of the interpolation, and add it back in later.
		const FVector DirectionVectorNormalized = DirectionVector.GetSafeNormal();
		return FMath::VInterpConstantTo(VelocityIn - VelocityVertical,
		                                FVector::DotProduct(VelocityIn, DirectionVectorNormalized) *
		                                DirectionVectorNormalized, DeltaTime, RunningFriction) + VelocityVertical;
	}
	return ApplySlidingFriction(VelocityIn, DeltaTime, BreakingFriction);
}


FVector UPupMovementComponent::ApplySlidingFriction(const FVector& VelocityIn, const float DeltaTime,
                                                    const float Friction) const
{
	const FVector VelocityVertical = FVector::DotProduct(FloorNormal, VelocityIn) * FloorNormal;
	return FMath::VInterpConstantTo(VelocityIn, VelocityVertical, DeltaTime, Friction);
}


void UPupMovementComponent::MagnetToBasis(const float VelocityFactor, const float DeltaTime)
{
	if (bGrounded && CurrentFloorComponent && CurrentFloorComponent->Mobility == EComponentMobility::Movable)
	{
		// Where is this local space vector in world space now? How has it moved in world space?
		const FVector FloorPositionAfterUpdate = LocalBasisPosition.X * CurrentFloorComponent->GetForwardVector() +
			LocalBasisPosition.Y * CurrentFloorComponent->GetRightVector() +
			LocalBasisPosition.Z * CurrentFloorComponent->GetUpVector() +
			CurrentFloorComponent->GetComponentLocation();

		const float DeltaYaw = CurrentFloorComponent->GetComponentRotation().Yaw - LastBasisRotation.Yaw;
		const FRotator DeltaRotation = FRotator(0.0f, DeltaYaw, 0.0f);

		DrawDebugLine(GetWorld(), UpdatedComponent->GetComponentLocation(), FloorPositionAfterUpdate, FColor::Purple, false, 20.0f, -2);
		BasisRelativeVelocity = FloorPositionAfterUpdate - UpdatedComponent->GetComponentLocation();
		UpdatedComponent->AddWorldOffset(BasisRelativeVelocity * VelocityFactor, true);
		BasisRelativeVelocity /= DeltaTime;
		DesiredRotation += DeltaRotation;
		UpdatedComponent->AddWorldRotation(DeltaRotation * VelocityFactor);
	}
	else
	{
		BasisRelativeVelocity = FVector::ZeroVector;
		LastBasisPosition = UpdatedComponent->GetComponentLocation();
		LastBasisRotation = DesiredRotation;
	}
}


void UPupMovementComponent::StoreBasisTransformPostUpdate()
{
	LastBasisPosition = UpdatedComponent->GetComponentLocation();
	// Don't bother storing this information if the object can't move
	if (bGrounded && CurrentFloorComponent && CurrentFloorComponent->Mobility == EComponentMobility::Movable)
	{
		LocalBasisPosition = GetRelativeBasisPosition();
		LastBasisRotation = CurrentFloorComponent->GetComponentRotation();
	}
}


FVector UPupMovementComponent::GetRelativeBasisPosition() const
{
	if (CurrentFloorComponent)
	{
		const FVector Distance = UpdatedComponent->GetComponentLocation() - CurrentFloorComponent->GetComponentLocation();
		
		return FVector(FVector::DotProduct(Distance, CurrentFloorComponent->GetForwardVector()),
			FVector::DotProduct(Distance, CurrentFloorComponent->GetRightVector()),
			FVector::DotProduct(Distance, CurrentFloorComponent->GetUpVector()));
	}
	return FVector::ZeroVector;
}


// Private impulse methods
FVector UPupMovementComponent::ConsumeImpulse()
{
	const FVector ConsumedVector = PendingImpulses;
	PendingImpulses = FVector::ZeroVector;
	return ConsumedVector;
}


void UPupMovementComponent::ClearImpulse()
{
	PendingImpulses = FVector::ZeroVector;
}


void UPupMovementComponent::AddAdjustment(const FVector& Adjustment)
{
	PendingAdjustments += Adjustment;
}


FVector UPupMovementComponent::ConsumeAdjustments()
{
	const FVector AdjustmentTotal = PendingAdjustments;
	PendingAdjustments = FVector::ZeroVector;
	return AdjustmentTotal;
}


// Utilities
FVector UPupMovementComponent::ClampToPlaneMaxSize(const FVector& VectorIn, const FVector& Normal, const float MaxSize)
{
	FVector Planar = VectorIn;
	const FVector VectorAlongNormal = FVector::DotProduct(Planar, Normal) * Normal;
	Planar -= VectorAlongNormal;

	if (Planar.Size() > MaxSize)
	{
		Planar = Planar.GetSafeNormal() * MaxSize;
	}
	return Planar + VectorAlongNormal;
}


bool UPupMovementComponent::SweepCapsule(const FVector Offset, FHitResult& OutHit, const bool bIgnoreInitialOverlap) const
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
		QueryParams.bFindInitialOverlaps = !bIgnoreInitialOverlap;

		for (UPrimitiveComponent* Component : IgnoredComponentsForSweep)
		{
			QueryParams.AddIgnoredComponent(Component);
		}

		const FCollisionResponseParams ResponseParams = FCollisionResponseParams::DefaultResponseParam;

		return World->SweepSingleByChannel(OutHit, Start, End, Capsule->GetComponentQuat(), ECC_Pawn,
										Capsule->GetCollisionShape(), QueryParams, ResponseParams);
	}
	return false;
}


void UPupMovementComponent::RenderHitResult(const FHitResult& HitResult, const FColor Color) const
{
	if (GEngine->GameViewport && GEngine->GameViewport->EngineShowFlags.Collision)
	{
		if (HitResult.GetComponent())
		{
			DrawDebugLine(GetWorld(), HitResult.ImpactPoint, HitResult.ImpactPoint + HitResult.Normal * 100.0f, Color,
	false, 0.02f, 1.0f, 2.0f);
			DrawDebugLine(GetWorld(), HitResult.ImpactPoint, HitResult.ImpactPoint + HitResult.ImpactNormal * 100.0f, Color,
	false, 0.02f, 1.0f, 2.0f);
			DrawDebugString(GetWorld(), HitResult.ImpactPoint + HitResult.ImpactNormal * 100.0f,
			HitResult.GetComponent()->GetName(), nullptr, Color, 0.02f, false, 1.0f);
		}
	}
}


bool UPupMovementComponent::CheckFloorValidWithinRange(const float Range, const FHitResult& HitResult) const
{
	bool bResult = false;

	if (HitResult.GetComponent() && HitResult.GetComponent()->Mobility != EComponentMobility::Movable)
	{
		const FVector DirectionFromCenter = (HitResult.ImpactPoint - HitResult.Component->GetComponentLocation()).GetSafeNormal();
		const FVector NewSweepLocation = HitResult.ImpactPoint + DirectionFromCenter * Range;
		
		FHitResult NewHitResult;
		FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
		bResult = HitResult.GetComponent()->LineTraceComponent(NewHitResult, NewSweepLocation, NewSweepLocation + FVector::UpVector * -10.0f, QueryParams);
		// RenderHitResult(NewHitResult);
	}
	return bResult;
}


// ReSharper disable once CppMemberFunctionMayBeConst
// This function may be changed to modify the movementcomponent directly, and probably should not be const forever
void UPupMovementComponent::HandleExternalOverlaps(const float DeltaTime)
{
	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(UpdatedComponent))
	{
		if (OverlapTest(PrimitiveComponent->GetComponentLocation(), PrimitiveComponent->GetComponentQuat(), ECollisionChannel::ECC_WorldDynamic, PrimitiveComponent->GetCollisionShape(), GetOwner()))
		{
			FHitResult EscapeHit;
			const FVector StartLocation = UpdatedComponent->GetComponentLocation();
			const FVector EndLocation = StartLocation + FVector::UpVector * 50.0f;
			const FQuat Rotator = UpdatedComponent->GetComponentQuat();
			FCollisionQueryParams CollisionQueryParams = FCollisionQueryParams::DefaultQueryParam;
			CollisionQueryParams.AddIgnoredActor(GetOwner());
			CollisionQueryParams.AddIgnoredComponent(CurrentFloorComponent);

			// If we've somehow gotten trapped inside a component that should've prevented a hit, we can try and sweep
			// and that will give us enough information to escape.
			GetWorld()->SweepSingleByChannel(EscapeHit, StartLocation, EndLocation, Rotator, ECollisionChannel::ECC_WorldDynamic, PrimitiveComponent->GetCollisionShape(), CollisionQueryParams);
			if (EscapeHit.GetComponent())
			{
				FVector Adjustment = GetPenetrationAdjustment(EscapeHit);
				ResolvePenetration(Adjustment, EscapeHit, UpdatedComponent->GetComponentQuat());
			}
		}
	}
}
