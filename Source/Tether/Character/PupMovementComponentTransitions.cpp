#include "PupMovementComponent.h"

#include "AITypes.h"
#include "DrawDebugHelpers.h"
#include "TetherCharacter.h"
#include "Components/CapsuleComponent.h"

void UPupMovementComponent::SetDefaultMovementMode()
{
	FHitResult FloorResult;
	if ((FindFloor(10.f, FloorResult, 1) && Velocity.Z <= 0.0f) || bGrounded)
	{
		SetMovementMode(EPupMovementMode::M_Walking);
		SnapToFloor(FloorResult);
		bAttachedToBasis = true;
		BasisComponent = FloorResult.GetComponent();
		bCanJump = true;
	}
	else
	{
		SetMovementMode(EPupMovementMode::M_Falling);
	}
	bMantling = false;
	// In the event the input is still being supressed by glitch or otherwise, a transition should reset it.
	UnsupressInput();
}


bool UPupMovementComponent::Jump()
{
	if (bSupressingInput || MatchModes(MovementMode, {EPupMovementMode::M_Dragging, EPupMovementMode::M_Recover, EPupMovementMode::M_None}))
	{
		return false;
	}
	if (bMantling)
	{
		ClimbMantle();
		return false;
	}
	if (bCanJump)
	{
		bAttachedToBasis = false;
		AddImpulse(UpdatedComponent->GetUpVector() * (JumpInitialVelocity - Velocity.Z));
		
		JumpAppliedVelocity += JumpInitialVelocity;
		bCanJump = false;
		bJumping = true;

		ATetherCharacter* Character = Cast<ATetherCharacter>(GetPawnOwner());
		UCapsuleComponent* Capsule = Character->GetCapsuleComponent();

		float PlayerHeight = 0.0f;
		if (IsValid(Character) && Capsule)
		{
			PlayerHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
		JumpEvent.Broadcast(BasisPositionLastTick - PlayerHeight * UpdatedComponent->GetUpVector(), true);
		
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
	if (bCanDoubleJump)
	{
		Velocity.Z = JumpInitialVelocity;
		if (bIsWalking)
		{
			// Apply extra directional velocity
			AddImpulse(MaxAcceleration * DesiredRotation.RotateVector(FVector::ForwardVector) * InputFactor * DoubleJumpeAccelerationFactor);
		}
		UpdatedComponent->SetWorldRotation(DesiredRotation);
		
		JumpAppliedVelocity += JumpInitialVelocity;
		bCanDoubleJump = false;
		bJumping = true;

		ATetherCharacter* Character = Cast<ATetherCharacter>(GetPawnOwner());
		UCapsuleComponent* Capsule = Character->GetCapsuleComponent();

		float PlayerHeight = 0.0f;
		if (IsValid(Character) && Capsule)
		{
			PlayerHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
		JumpEvent.Broadcast(BasisPositionLastTick - PlayerHeight * UpdatedComponent->GetUpVector(), false);
		
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


void UPupMovementComponent::TryRegainControl()
{
	if (bCanRegainControl)
	{
		if (FVector::DotProduct(DeflectDirection, Velocity) <= KINDA_SMALL_NUMBER)
		{
			DeflectDirection = FVector::ZeroVector;
			if (MovementMode == EPupMovementMode::M_Deflected)
			{
				SetDefaultMovementMode();
			}
		}
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
		if (MovementMode == EPupMovementMode::M_Anchored)
		{
			BreakAnchor(true);
		}
		SetMovementMode(EPupMovementMode::M_Deflected);
		DeflectDirection = DeflectionVelocity.GetSafeNormal();
		const float DeflectTimeRemaining = DeflectTimerHandle.IsValid() ? World->GetTimerManager().GetTimerRemaining(DeflectTimerHandle) : 0.f;
		if (DeflectTimeRemaining < DeflectTime)
		{
			World->GetTimerManager().SetTimer(DeflectTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
			{
				TryRegainControl();
			}), DeflectTime, false);
		}
	}
	AddImpulse(DeflectionVelocity);
}


void UPupMovementComponent::AnchorToComponent(UPrimitiveComponent* AnchorTargetComponent, const FVector& Location)
{
	if (!AnchorTargetComponent)
	{
		return;
	}
	FVector AttachmentLocation;
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();

	bAttachedToBasis = true;
	BasisComponent = AnchorTargetComponent;

	if (Location == FVector::ZeroVector)
	{
		FHitResult AnchorTestHit;
		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(UpdatedComponent);
		AnchorTargetComponent->GetClosestPointOnCollision(UpdatedComponent->GetComponentLocation(), AttachmentLocation);
		AnchorTargetComponent->SweepComponent(AnchorTestHit, ComponentLocation,
			AttachmentLocation,
			UpdatedComponent->GetComponentQuat(),
			PrimitiveComponent->GetCollisionShape());		
		const FVector BasisWorldLocation = AnchorTestHit.Location + AnchorTestHit.ImpactNormal * 2.0f;
		const FVector Distance = BasisWorldLocation - BasisComponent->GetComponentLocation();
		LocalBasisPosition = FVector(
			FVector::DotProduct(Distance, BasisComponent->GetForwardVector()),
			FVector::DotProduct(Distance, BasisComponent->GetRightVector()),
			FVector::DotProduct(Distance, BasisComponent->GetUpVector()));
	}
	bIsWalking = false;
	SetMovementMode(EPupMovementMode::M_Anchored);
}


void UPupMovementComponent::BreakAnchor(const bool bForceBreak)
{
	// These should be thrown out, since any that occurred during the move are invalid
	ConsumeAdjustments();
	bAttachedToBasis = false;
	BasisComponent = nullptr;
	if (bForceBreak)
	{
		MovementMode = EPupMovementMode::M_Deflected;
	}
	else
	{
		SetDefaultMovementMode();
	}
	GetWorld()->GetTimerManager().SetTimer(MantleDebounceTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
	{;
		bCanMantle = true;
	}), 0.2f, false);
}


void UPupMovementComponent::Land(const FVector& FloorLocation, const float ImpactVelocity, UPrimitiveComponent* FloorComponent)
{
	SetMovementMode(EPupMovementMode::M_Walking);
	bCanJump = true;
	if (bDoubleJump)
	{
		bCanDoubleJump = true;
	}
	LandEvent.Broadcast(FloorLocation, ImpactVelocity, FloorComponent);
}


void UPupMovementComponent::Fall()
{
	bAttachedToBasis = false;
	BasisComponent = nullptr;
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
	BasisPositionLastTick = UpdatedComponent->GetComponentLocation();
	ClearImpulse();
	SetDefaultMovementMode();
}


void UPupMovementComponent::ClimbMantle()
{
	if (FVector::DotProduct(DirectionVector, -UpdatedComponent->GetForwardVector()) > 0.5f)
	{
		BreakAnchor(false);
	}
	else
	{
		MantleEvent.Broadcast();
		SupressInput();
	}
	bCanDoubleJump = true;
	bMantling = false;
}


void UPupMovementComponent::EndMantleClimb()
{
	BreakAnchor();
	UnsupressInput();
}


void UPupMovementComponent::BeginDraggingObject(const FVector& TargetNormal)
{
	// bIsDraggingSomething = true;
	DraggingFaceNormal = TargetNormal;
	SetMovementMode(EPupMovementMode::M_Dragging);
}

void UPupMovementComponent::EndDraggingObject()
{
	SetDefaultMovementMode();
}


bool UPupMovementComponent::EdgeSlide(const float Scale, const float DeltaTime)
{
	if (FMath::Abs(Scale) < 0.5f)
	{
		return false;
	}
	const float CurvedScale = Scale > 0.0f ? (Scale - 0.5f) * 2.0f : (Scale + 0.5f) * 2.0f;
	
	// TODO: Make this rate a UProperty?
	const float SlideRate = 100.0f;
	const float CapsuleRadius = 10.0f;

	const FVector RotatedLedgeDirection = BasisComponent->GetComponentRotation().RotateVector(LedgeDirection);
	
	const FVector SlideOffset = RotatedLedgeDirection * SlideRate * CurvedScale * DeltaTime;
	const FVector NewAnchorWorldLocation = UpdatedComponent->GetComponentLocation() + SlideOffset;
	const FVector RadiusCheckOffset = Scale > 0.0f ? RotatedLedgeDirection * CapsuleRadius : RotatedLedgeDirection * -CapsuleRadius;
	
	FHitResult LineTraceResult;
	if (BasisComponent && BasisComponent->LineTraceComponent(LineTraceResult,
		NewAnchorWorldLocation + RadiusCheckOffset,
		NewAnchorWorldLocation + RadiusCheckOffset + UpdatedComponent->GetForwardVector() * GrabRangeForward,
		FCollisionQueryParams::DefaultQueryParam))
	{
		UpdatedComponent->AddWorldOffset(SlideOffset);
		TurningDirection = CurvedScale;
		return true;
	}
	return false;
}


void UPupMovementComponent::Mantle()
{
	if (UpdatedComponent->GetClass() != UCapsuleComponent::StaticClass() || !bCanMantle)
	{
		return;
	}
	UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(UpdatedComponent);
	const float CapsuleRadius = CapsuleComponent->GetScaledCapsuleRadius();
	
	const FVector EyePosition = CapsuleComponent->GetComponentLocation() +
		(CapsuleComponent->GetUnscaledCapsuleHalfHeight() - CapsuleRadius - 8.0f) * FVector::UpVector +
		CapsuleRadius * CapsuleComponent->GetForwardVector();
	FHitResult LineTraceResult;
	
	FCollisionQueryParams CollisionQueryParams = FCollisionQueryParams::DefaultQueryParam;
	CollisionQueryParams.AddIgnoredActor(this->GetOwner());
	
	if (GetWorld()->LineTraceSingleByChannel(LineTraceResult,
		EyePosition, EyePosition + CapsuleComponent->GetForwardVector() * GrabRangeForward,
		ECollisionChannel::ECC_Pawn,
		CollisionQueryParams,
		CapsuleComponent->GetCollisionResponseToChannels()))
	{
		// Found object in front of the player... we need to sweep again
		if (!LineTraceResult.GetComponent() || !LineTraceResult.GetComponent()->CanCharacterStepUp(this->GetPawnOwner()))
		{
			return;
		}
		UPrimitiveComponent* Target = LineTraceResult.GetComponent();
		const float Alpha = 1.0f;
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-LineTraceResult.ImpactNormal.Y, -LineTraceResult.ImpactNormal.X));
		
		const FVector MantleLocation = LineTraceResult.ImpactPoint - LineTraceResult.ImpactNormal * Alpha;
		const FVector WallNormal = LineTraceResult.Normal;

		if (bDrawMovementDebug && DrawDebugLayers[TEXT("MantleHitResults")])
		{
			DrawDebugPoint(GetWorld(), MantleLocation, 5.0f, FColor::Purple, false, 1.0f, -1);
		}

		
		if (Target->LineTraceComponent(LineTraceResult,
			MantleLocation + FVector::UpVector * GrabRangeTop, MantleLocation + FVector::DownVector * GrabRangeBottom,
			CollisionQueryParams) && !LineTraceResult.bStartPenetrating)
		{
			const FVector TopWallNormal = LineTraceResult.Normal;
			LedgeDirection = FVector::CrossProduct(TopWallNormal, WallNormal).GetUnsafeNormal();

			/* Check how close our ledge direction vector is to the 'right vector',
			 * assuming that the 'forward vector' is the direction the player will rotate towards --
			 * the opposite of the WallNormal */
			if (FMath::Abs(FVector::DotProduct(LedgeDirection, WallNormal.RotateAngleAxis(90.0f, FVector::UpVector))) >= LedgeDeviation)
			{
				const FVector LeftSide = UpdatedComponent->GetComponentLocation() - LedgeDirection * CapsuleComponent->GetCollisionShape().GetCapsuleRadius();
				const FVector RightSide = UpdatedComponent->GetComponentLocation() + LedgeDirection * CapsuleComponent->GetCollisionShape().GetCapsuleRadius();
				const FVector Offset =  (MantleLocation - UpdatedComponent->GetComponentLocation()).GetSafeNormal2D() * GrabRangeForward;

				FHitResult SizeTraceLeft;
				FHitResult SizeTraceRight;
				if (!Target->LineTraceComponent(SizeTraceLeft, LeftSide, LeftSide + Offset, CollisionQueryParams) || !Target->LineTraceComponent(SizeTraceRight, RightSide, RightSide + Offset, CollisionQueryParams))
				{
					return;
				}
				const FVector AnchorWorldLocation = LineTraceResult.Location + (CapsuleComponent->GetComponentLocation() - EyePosition);
				const FVector Difference = AnchorWorldLocation - Target->GetComponentLocation();

				if (bDrawMovementDebug && DrawDebugLayers[TEXT("MantleHitResults")])
				{
					DrawDebugPoint(GetWorld(), AnchorWorldLocation, 5.0f, FColor::Purple, false, 1.0f, -1);
				}
				
				if (Target->Mobility == EComponentMobility::Movable)
				{
					LedgeDirection = Target->GetComponentRotation().GetInverse().RotateVector(LedgeDirection);
				}				
				LocalBasisPosition = FVector(
					FVector::DotProduct(Difference, Target->GetForwardVector()),
					FVector::DotProduct(Difference, Target->GetRightVector()),
					FVector::DotProduct(Difference, Target->GetUpVector()));

				BasisRotationLastTick = Target->GetComponentRotation();
				
				bMantling = true;
				bCanMantle = false;
				bIsWalking = false;
				bAttachedToBasis = true;
				BasisComponent = Target;
				SetMovementMode(EPupMovementMode::M_Anchored);

				UpdatedComponent->SetWorldLocation(AnchorWorldLocation);
				UpdatedComponent->SetWorldRotation(FRotator(0.0f, Yaw, 0.0f));
				DesiredRotation = FRotator(0.0f, Yaw, 0.0f);
			}
		}
	}
}
