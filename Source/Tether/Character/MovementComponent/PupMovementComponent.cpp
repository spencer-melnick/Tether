// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "PupMovementComponent.h"

#include "DrawDebugHelpers.h"
#include "../TetherCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Tether/Tether.h"
#include "Tether/Gameplay/Obstacles/Conveyor.h"


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
	
	static float KillZ = -100.0f;
	static FAutoConsoleVariableRef CVarKillZ(
		TEXT("PupMovement.KillZ"),
		KillZ,
		TEXT("Minimum Z value to kill the player"),
		ECVF_Default);
}


UPupMovementComponent::UPupMovementComponent()
{}

UPupMovementComponent::UPupMovementComponent(const FObjectInitializer& ObjectInitializer)
{
}


void UPupMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	SetDefaultMovementMode();
	DesiredRotation = UpdatedComponent->GetComponentRotation();
	BasisPositionLastTick = UpdatedComponent->GetComponentLocation();
	LastValidLocation = UpdatedComponent->GetComponentLocation();
}


void UPupMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* TickFunction)
{
	// Gather all of the input we've accumulated since the last frame
	HandleInputVectors();
	
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
	/* if (MatchModes(MovementMode, {EPupMovementMode::M_Walking, EPupMovementMode::M_Falling, EPupMovementMode::M_Deflected}))
	{
		if (Hit.GetComponent())
		{
			// We shouldn't copy the velocity from slight floor glitches -- this could lead to the player
			// being counted as being in the air.
			const FVector AdjustmentWithoutBasis = Adjustment - FVector::DotProduct(Adjustment, FloorNormal) * FloorNormal;
			if (AdjustmentWithoutBasis.Size() >= KINDA_SMALL_NUMBER)
			{
				// The further in the sweep the move had to occur, the faster we had to move out of the way
				// AddAdjustment(AdjustmentWithoutBasis / (1 - Hit.Time));
				if (bDrawMovementDebug)
				{
					DrawDebugDirectionalArrow(GetWorld(), UpdatedComponent->GetComponentLocation(),
						UpdatedComponent->GetComponentLocation() + Adjustment * 10.0f, 5.0f, FColor::Green, false, 10.0f);
				}
			}
			RenderHitResult(Hit, FColor::Green);
			InvalidFloorComponents.Add(Hit.GetComponent());
		}
	} */
	// return Super::ResolvePenetrationImpl(Adjustment, Hit, NewRotation);
	return true;
}


void UPupMovementComponent::StepMovement(const float DeltaTime)
{
	MagnetToBasis(1.0f, DeltaTime);
	// HandleExternalOverlaps(DeltaTime);
	UpdatedComponent->SetWorldRotation(GetNewRotation(DeltaTime));
	Velocity = GetNewVelocity(DeltaTime);
	HandlePushes(DeltaTime);
	
	if (MatchModes(MovementMode, {EPupMovementMode::M_Walking, EPupMovementMode::M_Falling, EPupMovementMode::M_Deflected, EPupMovementMode::M_Anchored, EPupMovementMode::M_Dragging}))
	{
		UpdateVerticalMovement(DeltaTime);
		TryRegainControl();

		float NewDeltaTime = DeltaTime;
		// Break up physics into substeps based on collisions
		int32 NumSubsteps = 0;
		while (NumSubsteps < PupMovementCVars::MaxSubsteps && DeltaTime > SMALL_NUMBER)
		{
			NumSubsteps++;
			NewDeltaTime -= SubstepMovement(NewDeltaTime);
		}
		if (UpdatedComponent->GetComponentLocation().Z <= PupMovementCVars::KillZ)
		{
			Recover();
		}
	}
	else if (MovementMode == EPupMovementMode::M_Recover && bIgnoreObstaclesWhenRecovering)
	{
		UpdatedComponent->SetWorldLocation(UpdatedComponent->GetComponentLocation() + Velocity * DeltaTime, false);
	}

	MovementSpeedAlpha = Velocity.IsNearlyZero() ? 0.0f : Velocity.Size2D() / MaxSpeed;
	InvalidFloorComponents.Empty();
	
	// Let our primitive component know what its new velocity should be
	UpdateComponentVelocity();

	// EXPERIMENTAL!!!!
	HandleRootMotion();
	
	StoreBasisTransformPostUpdate();
}


float UPupMovementComponent::SubstepMovement(const float DeltaTime)
{
	const FVector Movement = Velocity * DeltaTime;

	if (Movement.IsNearlyZero())
	{
		// Don't move...
		return DeltaTime;
	}
	

	// Sweep before attempting to move?
	FHitResult HitResult;
	SweepCapsule(Movement, HitResult, false);
	if (HitResult.bStartPenetrating)
	{
		// Calculate the necessary adjustment to get out of whatever object we're in
		const FVector Adjustment = GetPenetrationAdjustment(HitResult);
		UpdatedComponent->AddWorldOffset(Adjustment, false);
	}
	else if (!HitResult.bBlockingHit)
	{
		// Move normally
		UpdatedComponent->SetWorldLocation(HitResult.TraceEnd, false);
	}

	
	// Remember any penetration adjustments we had to do, and push the player next frame
	// Convert our adjustment into a new impulse, based on the DeltaTime that we had when it occurred
	const FVector AdjustmentVelocity = ConsumeAdjustments() / DeltaTime;
	if (!AdjustmentVelocity.IsNearlyZero())
	{
		// AddImpulse(AdjustmentVelocity / 2);
	}
	
	// If we hit something...
	if (HitResult.bBlockingHit)
	{
		if (!HitResult.bStartPenetrating)
		{
			UpdatedComponent->SetWorldLocation(HitResult.Location + HitResult.ImpactNormal * 0.1f);
		}
		
		FVector RelativeVelocity = Velocity;
		if (HitResult.GetComponent())
		{
			RelativeVelocity -= HitResult.GetComponent()->GetComponentVelocity();
		}
		const float ImpactVelocityMagnitude = FMath::Min(FVector::DotProduct(HitResult.Normal, RelativeVelocity), 0.f);
		
		Velocity -= HitResult.Normal * ImpactVelocityMagnitude;
		
		return HitResult.Time * DeltaTime;
	}
	// Completed the move with no collisions!
	return DeltaTime;
}


void UPupMovementComponent::UpdateVerticalMovement(const float DeltaTime)
{
	// First check if we're on the floor
	FHitResult FloorHit;
	const float EffectiveSnapDistance = FMath::Max(FloorSnapDistance, Velocity.Z * -DeltaTime);
	if (FindFloor(EffectiveSnapDistance, FloorHit, 2))
	{
		const FVector RelativeVelocity = Velocity - FloorHit.GetComponent()->GetComponentVelocity();
		if (FVector::DotProduct(RelativeVelocity, FloorNormal) <= KINDA_SMALL_NUMBER) // Avoid floating point errors..
		{
			const FVector ImpactVelocity = FVector::DotProduct(RelativeVelocity, FloorNormal) * FloorNormal;
			if (!bGrounded && MovementMode == EPupMovementMode::M_Falling)
			{
				Land(FloorHit.ImpactPoint, ImpactVelocity.Size(), FloorHit.GetComponent());
			}
			bGrounded = true;
			SnapToFloor(FloorHit);
			if (MovementMode == EPupMovementMode::M_Walking)
			{
				bAttachedToBasis = true;
				BasisComponent = FloorHit.GetComponent();
			}
		}
		if (MatchModes(MovementMode, {EPupMovementMode::M_Anchored}))
		{
			const FVector ImpactVelocity = FVector::DotProduct(RelativeVelocity, FloorNormal) * FloorNormal;
			Land(FloorHit.ImpactPoint, ImpactVelocity.Size(), FloorHit.GetComponent());
		}
	}
	else
	{
		if (bGrounded && MovementMode == EPupMovementMode::M_Walking)
		{
			Fall();
		}
		bGrounded = false;
	}
	if (MovementMode == EPupMovementMode::M_Falling && (Velocity.Z <= -MaximumGrabVelocity || bWallScrambling) )
	{
		Mantle();
	}
	if (!bGrounded && MovementMode != EPupMovementMode::M_Anchored)
	{
		// To avoid potential stuttering, only apply gravity if we're not on the ground
		Velocity.Z += GetGravityZ() * DeltaTime;
	}

	if (bWallSliding)
	{
		// Verify we are actually adjacent to the wall with a line trace
		FHitResult LineTrace;
		if (BasisComponent && BasisComponent->LineTraceComponent(LineTrace, UpdatedComponent->GetComponentLocation(),
			UpdatedComponent->GetComponentLocation() - WallNormal * 100.0f,
			FCollisionQueryParams::DefaultQueryParam))
		{
			Velocity = FMath::VInterpConstantTo(Velocity, BasisComponent->ComponentVelocity, DeltaTime, BreakingFriction * 0.25f);
		}
		else
		{
			bWallSliding = false;
		}
	}
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
			AddImpulse(BasisRelativeVelocity);
			break;
		}
	default:
		break;
	}
	MovementModeChanged.Broadcast(MovementMode, NewMovementMode);
	MovementMode = NewMovementMode;
	return true;
}


EPupMovementMode UPupMovementComponent::GetMovementMode() const
{
	return MovementMode;
}


void UPupMovementComponent::AddImpulse(const FVector Impulse)
{
	PendingImpulses += Impulse;
}


void UPupMovementComponent::Push(const FHitResult& HitResult, const FVector ImpactVelocity, UPrimitiveComponent* Source)
{
	const FVector Normal = -HitResult.ImpactNormal;
	FVector Adjustment = (FVector::DotProduct(HitResult.TraceEnd - HitResult.Location, Normal) + 0.1f) * Normal;
	if (HitResult.bStartPenetrating)
	{
		Adjustment -= HitResult.PenetrationDepth * Velocity.GetSafeNormal();
	}
	// UpdatedComponent->AddWorldOffset(Adjustment);
	if (MovementMode == EPupMovementMode::M_Anchored)
	{
		BreakAnchor();
	}
	UpdatedComponent->AddWorldOffset(Normal * 0.1f);
	PendingPushes = ImpactVelocity;
}


void UPupMovementComponent::IgnoreActor(AActor* Actor)
{
	if (Actor)
	{
		IgnoredActors.Add(Actor);
	}
}


void UPupMovementComponent::UnignoreActor(AActor* Actor)
{
	if (Actor)
	{
		IgnoredActors.Remove(Actor);
	}
}


void UPupMovementComponent::SupressInput()
{
	bSupressingInput = true;
}


void UPupMovementComponent::UnsupressInput()
{
	bSupressingInput = false;
}


void UPupMovementComponent::HandleInputVectors()
{
	// Copy the camera yaw
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
	if (!InputVector.IsNearlyZero() && !bSupressingInput)
	{
		// Clamp input axes
		InputFactor = FMath::Min(InputVector.Size(), 1.0f);
		bIsWalking = true;
		const float CameraYawRads = FMath::DegreesToRadians(CameraYaw);
		const float Sin = FMath::Sin(CameraYawRads);
		const float Cos = FMath::Cos(CameraYawRads);
		DirectionVector = FVector(
			Cos * InputVector.X - Sin * InputVector.Y,
			Sin * InputVector.X + Cos * InputVector.Y,
			0.0f);
		DirectionVector = DirectionVector.GetClampedToMaxSize(1.0f);
	}
	else
	{
		InputFactor = 0.0f;
		bIsWalking = false;
		DirectionVector = FVector::ZeroVector;
	}
}


// Update method wrappers
FRotator UPupMovementComponent::GetNewRotation(const float DeltaTime)
{
	switch (MovementMode)
	{
	case EPupMovementMode::M_Walking:
		{
			DesiredRotation.Yaw = bIsWalking ?
				FMath::RadiansToDegrees(FMath::Atan2(DirectionVector.Y, DirectionVector.X)) :
				UpdatedComponent->GetComponentRotation().Yaw;
			float RotationRate = RotationSpeed;
			if (bSlip)
			{
				RotationRate *= 1 - (1 - SlipFactor) * MovementSpeedAlpha;
			}
			const FRotator NewRotation = FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime, RotationRate);
			const float DeltaYaw = FMath::FindDeltaAngleDegrees(DesiredRotation.Yaw, NewRotation.Yaw);

			TurningDirection = FMath::Abs(DeltaYaw) > 0.01f ?
				FMath::Clamp(FMath::Lerp(TurningDirection, DeltaYaw / 45.0f, 5.0f * DeltaTime),
					-1.0f, 1.0f) :
				FMath::Lerp(TurningDirection, 0.0f, 5.0f * DeltaTime);
			return NewRotation;
		}
	case EPupMovementMode::M_Falling:
		{
			if (bWallSliding)
			{
				return FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime,
					SnapRotationVelocity);
			}
			if (bIsWalking)
			{
				DesiredRotation.Yaw = FMath::RadiansToDegrees(FMath::Atan2(DirectionVector.Y, DirectionVector.X));
			}
			const float RotationRate = RotationSpeed * AirControlFactor;
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
			FVector AccelerationDirection = UpdatedComponent->GetForwardVector() * InputFactor;
			if (FVector::DotProduct(FVector::UpVector, FloorNormal) < 0.99f)
			{
				// Only bother calculating the accleration along the plane if it isn't directly up
				AccelerationDirection = FQuat::FindBetweenNormals(FVector::UpVector, FloorNormal).RotateVector(AccelerationDirection);
			}
			const FVector Acceleration = bIsWalking ? MaxAcceleration * AccelerationDirection + BreakingFriction * AccelerationDirection.GetSafeNormal() : FVector::ZeroVector;
			FVector NewVelocity = Velocity + Acceleration * DeltaTime;
			NewVelocity = ApplyFriction(NewVelocity, DeltaTime);
			NewVelocity = ClampToPlaneMaxSize(NewVelocity, FloorNormal, MaxSpeed);
			NewVelocity += ConsumeImpulse();

			Speed = NewVelocity.Size();
			return NewVelocity;
		}
	case EPupMovementMode::M_Falling:
		{
			// FVector Acceleration = MaxAcceleration * DirectionVector * DeltaTime * AirControlFactor;
			FVector DesiredPlanarVelocity = bIsWalking ? DirectionVector * MaxSpeed : FVector::ZeroVector;
			const float Acceleration = MaxAcceleration * AirControlFactor;

			if (bWallJumpDisabledControl)
			{
				// Limit the player's control when they jump away from the wall
				DesiredPlanarVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
			}

			FVector NewVelocity = FMath::VInterpConstantTo(Velocity, DesiredPlanarVelocity + FVector(0.0f, 0.0f, Velocity.Z), DeltaTime, Acceleration);
			if (bWallSliding)
			{
				const float VelocityAwayFromWallScalar = FVector::DotProduct(NewVelocity, WallNormal);
				const FVector VelocityAwayFromWall = VelocityAwayFromWallScalar * WallNormal;

				if (VelocityAwayFromWallScalar > 0.0f)
				{
					NewVelocity -= VelocityAwayFromWall;
				}
				else if (VelocityAwayFromWallScalar < -1.0f)
				{
					if (bCanScramble)
					{
						bCanScramble = false;
						bWallScrambling = true;
						GetWorld()->GetTimerManager().SetTimer(EdgeScrambleTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
						{
							bWallScrambling = false;
						}), WallScrambleTime, false);
					}
					if (bWallScrambling)
					{
						NewVelocity.Z = FMath::Min(TerminalVelocity, NewVelocity.Z + 2000.0f * DeltaTime);
					}
				}
			}
			
			if (bJumping)
			{
				NewVelocity += HoldJump(DeltaTime);
			}
			return NewVelocity + ConsumeImpulse();
		}
	case EPupMovementMode::M_Anchored:
		{
			if (!IsValid(BasisComponent))
			{
				BreakAnchor(false);
			}
			FVector NewVelocity = FVector::ZeroVector;
			const FVector DistanceFromDesiredLocation = DesiredAnchorLocation - UpdatedComponent->GetComponentLocation();
			const FVector DirectionToDesiredLocation = DistanceFromDesiredLocation.GetSafeNormal();
			
			if (DistanceFromDesiredLocation.Size() > SnapVelocity * DeltaTime)
			{
				NewVelocity = DirectionToDesiredLocation * SnapVelocity;
			}
			else
			{
				UpdatedComponent->SetWorldLocation(DesiredAnchorLocation);
			}
			if (bMantling)
			{
				bIsWalking = EdgeSlide(FVector::DotProduct(BasisComponent->GetComponentRotation().RotateVector(LedgeDirection), DirectionVector), DeltaTime);
			}
			return NewVelocity;
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
			return Velocity;
		}
	case EPupMovementMode::M_Dragging:
		{
			const FVector RightVector = FVector::CrossProduct(DraggingFaceNormal, FVector::UpVector);
			const FVector NonPlanarVelocity = DirectionVector * DragSpeed - (FVector::DotProduct(RightVector, DirectionVector) * RightVector);
			if (!(FloorNormal - FVector::UpVector).IsNearlyZero())
			{
				return FQuat::FindBetweenNormals(FVector::UpVector, FloorNormal).RotateVector(NonPlanarVelocity);
			}
			return NonPlanarVelocity;
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


FVector UPupMovementComponent::ApplyFriction(const FVector& VelocityIn, const float DeltaTime)
{
	const FVector Result = FMath::VInterpConstantTo(VelocityIn, FVector::ZeroVector, DeltaTime, BreakingFriction);
	return Result;
}


FVector UPupMovementComponent::ApplySlidingFriction(const FVector& VelocityIn, const float DeltaTime, const float Friction) const
{
	const FVector VelocityVertical = FVector::DotProduct(FloorNormal, VelocityIn) * FloorNormal;
	return FMath::VInterpConstantTo(VelocityIn, VelocityVertical, DeltaTime, Friction);
}


void UPupMovementComponent::MagnetToBasis(const float VelocityFactor, const float DeltaTime)
{
	if (bAttachedToBasis && BasisComponent && BasisComponent->Mobility == EComponentMobility::Movable)
	{
		// Where is this local space vector in world space now? How has it moved in world space?
		const FVector PositionAfterUpdate =
			LocalBasisPosition.X * BasisComponent->GetForwardVector() +
			LocalBasisPosition.Y * BasisComponent->GetRightVector() +
			LocalBasisPosition.Z * BasisComponent->GetUpVector() +
			BasisComponent->GetComponentLocation();

		float DeltaYaw = FMath::FindDeltaAngleDegrees(BasisRotationLastTick.Yaw, BasisComponent->GetComponentRotation().Yaw);
		if (MovementMode == EPupMovementMode::M_Anchored)
		{
			BasisRelativeVelocity = PositionAfterUpdate - DesiredAnchorLocation;
		}
		else
		{
			BasisRelativeVelocity = PositionAfterUpdate - UpdatedComponent->GetComponentLocation();
		}
		if (!BasisRelativeVelocity.IsNearlyZero())
		{
			if (MovementMode == EPupMovementMode::M_Anchored)
			{
				UpdatedComponent->AddWorldOffset(PositionAfterUpdate - DesiredAnchorLocation);
				DesiredAnchorLocation = PositionAfterUpdate;
			}
			else
			{
				FHitResult MagnetMovementTestResult;
				if (!SweepCapsule(PositionAfterUpdate - UpdatedComponent->GetComponentLocation(), MagnetMovementTestResult))
				{
					UpdatedComponent->SetWorldLocation(PositionAfterUpdate);
				}
				else
				{
					UpdatedComponent->SetWorldLocation(MagnetMovementTestResult.Location);
					DeltaYaw *= MagnetMovementTestResult.Time;
				}
			}
			BasisRelativeVelocity /= DeltaTime;
		}
		if (UConveyorComponent* Conveyor = Cast<UConveyorComponent>(BasisComponent))
		{
			UpdatedComponent->AddWorldOffset(Conveyor->GetAppliedVelocity() * DeltaTime);
			BasisRelativeVelocity += Conveyor->GetAppliedVelocity();
		}
		DesiredRotation.Yaw = DesiredRotation.Yaw + DeltaYaw;
		FRotator NewRotation = UpdatedComponent->GetComponentRotation();
		NewRotation.Yaw += DeltaYaw;
		UpdatedComponent->SetWorldRotation(NewRotation);
	}
	else
	{
		BasisRelativeVelocity = FVector::ZeroVector;
		BasisPositionLastTick = UpdatedComponent->GetComponentLocation();
		BasisRotationLastTick = DesiredRotation;
		LocalBasisPosition = FVector::ZeroVector;
	}
}


void UPupMovementComponent::HandlePushes(const float DeltaTime)
{
	if (MatchModes(MovementMode, {EPupMovementMode::M_Walking, EPupMovementMode::M_Falling}))
	{
		if (!PendingPushes.IsNearlyZero())
		{
			const FVector PushDirection = PendingPushes.GetSafeNormal();
			const float VelocityInPushDirection = FVector::DotProduct(PushDirection, Velocity);
			if (VelocityInPushDirection < PendingPushes.Size())
			{
				Velocity -= VelocityInPushDirection * PushDirection;
				Velocity += PendingPushes;
			}
			PendingPushes = FVector::ZeroVector;
		}
	}
}


void UPupMovementComponent::StoreBasisTransformPostUpdate()
{
	if (bAttachedToBasis && BasisComponent && BasisComponent->Mobility == EComponentMobility::Movable)
	{
		BasisPositionLastTick = MovementMode == EPupMovementMode::M_Anchored ? DesiredAnchorLocation : UpdatedComponent->GetComponentLocation();
		// Don't bother storing this information if the object can't move
		LocalBasisPosition = GetRelativeBasisPosition();
		BasisRotationLastTick = BasisComponent->GetComponentRotation();
	}
}


FVector UPupMovementComponent::GetRelativeBasisPosition() const
{
	if (bAttachedToBasis)
	{
		FVector Location = UpdatedComponent->GetComponentLocation();
		if (MovementMode == EPupMovementMode::M_Anchored)
		{
			Location = DesiredAnchorLocation;
		}
		return GetLocationRelativeToComponent(Location, BasisComponent);
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