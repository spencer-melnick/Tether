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

	static float FloorPadding = 0.01f;
	static FAutoConsoleVariableRef CVarFloorPadding(
		TEXT("PupMovement.FloorPadding"),
		FloorPadding,
		TEXT("The minimum distance when snapping to the floor."),
		ECVF_Default);
	
	static float KillZ = -100.0f;
	static FAutoConsoleVariableRef CVarKillZ(
		TEXT("PupMovement.KillZ"),
		KillZ,
		TEXT("Minimum Z value to kill the player"),
		ECVF_Default);

	static bool GBDrawMovementDebug = false;
	static FAutoConsoleVariableRef CVarGBDrawMovementDebug(
		TEXT("PupMovement.GBDrawMovementDebug"),
		GBDrawMovementDebug,
		TEXT("Draw lines for debugging the movement?"),
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
	if (PupMovementCVars::GBDrawMovementDebug)
	{
		DrawDebugDirectionalArrow(GetWorld(), UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentLocation() + Adjustment * 1.0f, 5.0f, FColor::Blue, false, 10.0f);
		DrawDebugString(GetWorld(), UpdatedComponent->GetComponentLocation() + FVector(0.0f, 0.0f, 20.0f), UpdatedComponent->ComponentVelocity.ToString(), nullptr, FColor::Blue, 5.0f);
	}
	if (MatchModes(MovementMode, {EPupMovementMode::M_Walking, EPupMovementMode::M_Falling, EPupMovementMode::M_Deflected}))
	{
		if (Hit.GetComponent())
		{
			// We shouldn't copy the velocity from slight floor glitches -- this could lead to the player
			// being counted as being in the air.
			const FVector AdjustmentWithoutBasis = Adjustment - FVector::DotProduct(Adjustment, FloorNormal) * FloorNormal;
			if (AdjustmentWithoutBasis.Size() >= KINDA_SMALL_NUMBER)
			{
				// The further in the sweep the move had to occur, the faster we had to move out of the way
				AddAdjustment(AdjustmentWithoutBasis / (1 - Hit.Time));
				if (PupMovementCVars::GBDrawMovementDebug)
				{
					DrawDebugDirectionalArrow(GetWorld(), UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentLocation() + Adjustment * 10.0f, 5.0f, FColor::Green, false, 10.0f);
				}
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
	HandleExternalOverlaps(DeltaTime);
	UpdateRotation(DeltaTime);
	Velocity = GetNewVelocity(DeltaTime);

	if (MatchModes(MovementMode, {EPupMovementMode::M_Walking, EPupMovementMode::M_Falling, EPupMovementMode::M_Deflected}))
	{
		UpdateVerticalMovement(DeltaTime);
		TryRegainControl();

		// Velocity = Velocity.GetClampedToMaxSize(TerminalVelocity);
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


void UPupMovementComponent::UpdateVerticalMovement(const float DeltaTime)
{
	// First check if we're on the floor
	FHitResult FloorHit;
	if (FindFloor(10.0f, FloorHit, 2))
	{
		if (FVector::DotProduct(Velocity, FloorNormal) <= KINDA_SMALL_NUMBER) // Avoid floating point errors..
		{
			if (!bGrounded && MovementMode == EPupMovementMode::M_Falling)
			{
				Land();
			}
			bGrounded = true;
			Velocity -= FVector::DotProduct(Velocity, FloorNormal) * FloorNormal;
			SnapToFloor(FloorHit);
		}
		else
		{
			bGrounded = false;
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
	}
	if (!bGrounded)
	{
		// To avoid potential stuttering, only apply gravity if we're not on the ground
		Velocity.Z += GetGravityZ() * DeltaTime;
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
	UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(UpdatedComponent);
	AnchorTargetComponent->SweepComponent(AnchorTestHit, UpdatedComponent->GetComponentLocation(),
		AnchorTargetComponent->GetComponentLocation(),
		UpdatedComponent->GetComponentQuat(),
		PrimitiveComponent->GetCollisionShape());

	AnchorTarget = AnchorTargetComponent;
	// Anchor slightly away from the exact hit location to prevent getting stuck inside the object.
	AnchorWorldLocation = AnchorTestHit.Location + AnchorTestHit.ImpactNormal * 2.0f;
	
	if (AnchorTargetComponent->Mobility == EComponentMobility::Movable)
	{
		const FVector Distance = AnchorWorldLocation - AnchorTarget->GetComponentLocation();
		AnchorRelativeLocation =  FVector(FVector::DotProduct(Distance, AnchorTarget->GetForwardVector()),
			FVector::DotProduct(Distance, AnchorTarget->GetRightVector()),
			FVector::DotProduct(Distance, AnchorTarget->GetUpVector()));
	}
	bIsWalking = false;
	SetMovementMode(EPupMovementMode::M_Anchored);
}


void UPupMovementComponent::AddImpulse(const FVector Impulse)
{
	PendingImpulses += Impulse;
}


bool UPupMovementComponent::FindFloor(const float SweepDistance, FHitResult& OutHitResult, const int NumTries)
{
	const FVector SweepOffset = FVector::DownVector * SweepDistance;
	for (int i = 0; i < NumTries; i++)
	{
		FHitResult IterativeHitResult;
		if (SweepCapsule(FVector(0.0f, 0.0f, 10.0f),SweepOffset, IterativeHitResult, false))
		{
			OutHitResult = IterativeHitResult;
			RenderHitResult(IterativeHitResult, FColor::White, true);
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
	UPrimitiveComponent* FloorComponent = FloorHit.GetComponent();
	if (FloorComponent && FloorComponent->CanCharacterStepUp(GetPawnOwner()))
	{
		// Capsule traces will give us ImpactNormals that are sometimes 'glancing' edges
		// so, the most realiable way of getting the floor's normal is with a line trace
		FHitResult NormalLineTrace;
		FloorComponent->LineTraceComponent(
			NormalLineTrace,
			UpdatedComponent->GetComponentLocation(),
			UpdatedComponent->GetComponentLocation() + FVector(0.0f, 0.0f, -250.0f),
			FCollisionQueryParams::DefaultQueryParam);
		RenderHitResult(NormalLineTrace, FColor::Red);
		if (NormalLineTrace.ImpactNormal.Z >= MaxInclineZComponent)
		{
			return true;
		}
	}
	// GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Floor found, but invalid."));
	return false;
}


void UPupMovementComponent::SnapToFloor(const FHitResult& FloorHit)
{
	FHitResult DiscardHit;
	if (CheckFloorValidWithinRange(MinimumSafeRadius, FloorHit))
	{
		LastValidLocation = FloorHit.Location;
	}
	SafeMoveUpdatedComponent(
		FloorHit.Location - UpdatedComponent->GetComponentLocation() + FloorHit.Normal * PupMovementCVars::FloorPadding,
		UpdatedComponent->GetComponentQuat(), true, DiscardHit, ETeleportType::None);
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


void UPupMovementComponent::TickGravity(const float DeltaTime)
{
	if (UWorld* World = GetWorld())
	{
		Velocity.Z = FMath::Max(Velocity.Z + World->GetGravityZ() * DeltaTime, TerminalVelocity);
	}
}


void UPupMovementComponent::HandleInputVectors()
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
	if (MatchModes(MovementMode, {EPupMovementMode::M_Walking, EPupMovementMode::M_Falling, EPupMovementMode::M_Deflected}))
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


void UPupMovementComponent::UpdateRotation(const float DeltaTime)
{
	const FRotator NewRotation = GetNewRotation(DeltaTime);
	if (UpdatedComponent)
	{
		const float DeltaYaw = FMath::FindDeltaAngleDegrees(NewRotation.Yaw, UpdatedComponent->GetComponentRotation().Yaw);
		TurningDirection = DeltaYaw;
		UpdatedComponent->SetWorldRotation(NewRotation);
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
			UpdatedComponent->SetWorldLocation(FMath::VInterpConstantTo(UpdatedComponent->GetComponentLocation(), AnchorWorldLocation, DeltaTime, SnapVelocity), false);
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
	if (MovementMode == EPupMovementMode::M_Anchored && AnchorTarget && AnchorTarget->Mobility == EComponentMobility::Movable)
	{
		AnchorWorldLocation = AnchorTarget->GetComponentLocation() + 
			AnchorRelativeLocation.X * AnchorTarget->GetForwardVector(),
			AnchorRelativeLocation.Y * AnchorTarget->GetRightVector(),
			AnchorRelativeLocation.Z * AnchorTarget->GetUpVector();
		return;
	}
	
	if (bGrounded && CurrentFloorComponent && CurrentFloorComponent->Mobility == EComponentMobility::Movable)
	{
		// Where is this local space vector in world space now? How has it moved in world space?
		const FVector FloorPositionAfterUpdate =
			LocalBasisPosition.X * CurrentFloorComponent->GetForwardVector() +
			LocalBasisPosition.Y * CurrentFloorComponent->GetRightVector() +
			LocalBasisPosition.Z * CurrentFloorComponent->GetUpVector() +
			CurrentFloorComponent->GetComponentLocation();

		const float DeltaYaw = CurrentFloorComponent->GetComponentRotation().Yaw - LastBasisRotation.Yaw;
		const FRotator DeltaRotation = FRotator(0.0f, DeltaYaw, 0.0f);

		BasisRelativeVelocity = FloorPositionAfterUpdate - UpdatedComponent->GetComponentLocation();
		UpdatedComponent->SetWorldLocation(FloorPositionAfterUpdate + FVector::UpVector * 1.0f);
		BasisRelativeVelocity /= DeltaTime;
		DesiredRotation += DeltaRotation;
		UpdatedComponent->AddWorldRotation(DeltaRotation * VelocityFactor);
	}
	else
	{
		BasisRelativeVelocity = FVector::ZeroVector;
		LastBasisPosition = UpdatedComponent->GetComponentLocation();
		LastBasisRotation = DesiredRotation;
		LocalBasisPosition = FVector::ZeroVector;
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
		
		return FVector(
			FVector::DotProduct(Distance, CurrentFloorComponent->GetForwardVector()),
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