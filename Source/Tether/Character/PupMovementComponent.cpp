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
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UPupMovementComponent, ApexVelocity) || PropertyName == GET_MEMBER_NAME_CHECKED(UPupMovementComponent, MaxJumpTime) || PropertyName == GET_MEMBER_NAME_CHECKED(UPupMovementComponent, JumpInitialVelocity))
	{
		HoldJumpAcceleration = MaxJumpTime <= 0.0f ? 0.0f : (ApexVelocity - JumpInitialVelocity) / MaxJumpTime;
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
		bCanJump = true;
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


bool UPupMovementComponent::FindFloor(float SweepDistance, FHitResult& OutHitResult)
{
	const FVector SweepOffset = FVector::DownVector * SweepDistance;
	if (SweepCapsule(SweepOffset, OutHitResult))
	{
		if (IsValidFloorHit(OutHitResult))
		{
			CurrentFloorComponent = OutHitResult.GetComponent();
			FloorNormal = OutHitResult.ImpactNormal;
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
	if (FloorComponent && FloorComponent->CanCharacterStepUp(GetPawnOwner()) && FloorHit.ImpactNormal.Z >= MaxInclineZComponent)
	{
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
		AddImpulse(UpdatedComponent->GetUpVector() * JumpInitialVelocity);
		JumpAppliedVelocity += JumpInitialVelocity;
		bCanJump = false;
		bJumping = true;

		if (UWorld* World = GetWorld())
		{
			if (MaxJumpTime > 0.0f)
			{
				World->GetTimerManager().SetTimer(JumpTimer, FTimerDelegate::CreateWeakLambda(this, [this]
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
		const float DeltaYaw = FMath::FindDeltaAngleDegrees(NewRotation.Yaw, UpdatedComponent->GetComponentRotation().Yaw);
		TurningDirection = DeltaYaw;
	
		UpdatedComponent->SetWorldRotation(NewRotation);
	}
	Velocity = GetNewVelocity(DeltaTime);
	
	if (MovementMode == EPupMovementMode::M_Walking ||  MovementMode == EPupMovementMode::M_Falling || MovementMode == EPupMovementMode::M_Deflected)
	{
		// First check if we're on the floor
		FHitResult FloorHit;

		if (FindFloor(10.f, FloorHit) &&
			IsValidFloorHit(FloorHit) &&
			FVector::DotProduct(Velocity, FloorNormal) <= KINDA_SMALL_NUMBER) // Avoid floating point errors..
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
		// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Substeps: %d"), NumSubsteps).Append(Velocity.ToString()));
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
			DesiredRotation = UpdatedComponent->GetComponentRotation();
		}
	}
}


FVector UPupMovementComponent::ClampToPlaneMaxSize(const FVector& VectorIn, const FVector& Normal, const float MaxSize) const
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
				DirectionVector = FQuat::FindBetweenNormals(FVector::UpVector, FloorNormal).RotateVector(DirectionVector);
				const FVector Acceleration = DirectionVector.IsNearlyZero() ? FVector::ZeroVector : MaxAcceleration * DirectionVector;
				FVector NewVelocity = Velocity + Acceleration * DeltaTime;

				NewVelocity = ClampToPlaneMaxSize(NewVelocity, FloorNormal, MaxSpeed);
				NewVelocity += ConsumeImpulse();
				NewVelocity = ApplyFriction(NewVelocity, DeltaTime);
				
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
				const FVector Offset = AnchorLocation - UpdatedComponent->GetComponentLocation();
				FVector NewVelocity = Offset.GetSafeNormal() * SnapVelocity;
				NewVelocity.Z = 0.0f;
				return NewVelocity;				
			}
		case EPupMovementMode::M_Deflected:
			{
				FVector NewVelocity = Velocity;
				if (bGrounded)
				{
					NewVelocity = ApplySlidingFriction(Velocity, DeltaTime, DeflectionFriction);
				}
				NewVelocity += DirectionVector.IsNearlyZero() ? FVector::ZeroVector : DeltaTime * MaxAcceleration * DirectionVector * DeflectionControlInfluence;
				return NewVelocity + ConsumeImpulse();
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
	if(bIsWalking)
	{
		const FVector VelocityVertical = FVector::DotProduct(FloorNormal, VelocityIn) * FloorNormal;

		// Take the Vertical Velocity (along the Floor Normal) out of the interpolation, and add it back in later.
		const FVector DirectionVectorNormalized = DirectionVector.GetSafeNormal();
		return FMath::VInterpConstantTo(VelocityIn - VelocityVertical, FVector::DotProduct(VelocityIn, DirectionVectorNormalized) * DirectionVectorNormalized, DeltaTime, RunningFriction) + VelocityVertical;
	}
	return ApplySlidingFriction(VelocityIn, DeltaTime, BreakingFriction);
}


FVector UPupMovementComponent::ApplySlidingFriction(const FVector& VelocityIn, const float DeltaTime,
	const float Friction) const
{
	const FVector VelocityVertical = FVector::DotProduct(FloorNormal, VelocityIn) * FloorNormal;
	return FMath::VInterpConstantTo(VelocityIn, VelocityVertical, DeltaTime, Friction);
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
