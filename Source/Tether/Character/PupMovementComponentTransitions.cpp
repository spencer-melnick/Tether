#include "PupMovementComponent.h"

#include "AITypes.h"
#include "DrawDebugHelpers.h"
#include "TetherCharacter.h"
#include "Components/CapsuleComponent.h"

bool UPupMovementComponent::Jump()
{
	if (MatchModes(MovementMode, {EPupMovementMode::M_Recover, EPupMovementMode::M_None}))
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
		JumpEvent.Broadcast(LastBasisPosition - PlayerHeight * UpdatedComponent->GetUpVector(), true);
		
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
		JumpEvent.Broadcast(LastBasisPosition - PlayerHeight * UpdatedComponent->GetUpVector(), false);
		
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

	AnchorTarget = AnchorTargetComponent;

	if (Location == FVector::ZeroVector)
	{
		FHitResult AnchorTestHit;
		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(UpdatedComponent);
		AnchorTargetComponent->GetClosestPointOnCollision(UpdatedComponent->GetComponentLocation(), AttachmentLocation);
		AnchorTargetComponent->SweepComponent(AnchorTestHit, ComponentLocation,
			AttachmentLocation,
			UpdatedComponent->GetComponentQuat(),
			PrimitiveComponent->GetCollisionShape());		
		AnchorWorldLocation = AnchorTestHit.Location + AnchorTestHit.ImpactNormal * 2.0f;
	}
	else
	{
		AnchorWorldLocation = Location;
	}
	const FVector Distance = AnchorWorldLocation - AnchorTarget->GetComponentLocation();
	AnchorRelativeLocation =  FVector(
		FVector::DotProduct(Distance, AnchorTarget->GetForwardVector()),
		FVector::DotProduct(Distance, AnchorTarget->GetRightVector()),
		FVector::DotProduct(Distance, AnchorTarget->GetUpVector()));
	const float NewAnchorYaw = FMath::RadiansToDegrees(FMath::Atan2(
		AttachmentLocation.Y - ComponentLocation.Y,
		AttachmentLocation.X - ComponentLocation.X));
	if (AnchorTargetComponent->Mobility == EComponentMobility::Movable)
	{
		bGrounded = false;
		AnchorRelativeYaw = NewAnchorYaw - AnchorTargetComponent->GetComponentRotation().Yaw;
	}
	DesiredRotation.Yaw = NewAnchorYaw;
	bIsWalking = false;
	SetMovementMode(EPupMovementMode::M_Anchored);
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
	GetWorld()->GetTimerManager().SetTimer(MantleDebounceTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
	{;
		bCanMantle = true;
	}), 0.2f, false);
}


void UPupMovementComponent::Land(const FVector& FloorLocation, const float ImpactVelocity)
{
	SetMovementMode(EPupMovementMode::M_Walking);
	bCanJump = true;
	if (bDoubleJump)
	{
		bCanDoubleJump = true;
	}
	LandEvent.Broadcast(FloorLocation, ImpactVelocity);
}


void UPupMovementComponent::Fall()
{
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


void UPupMovementComponent::ClimbMantle()
{
	if (FVector::DotProduct(DirectionVector, -UpdatedComponent->GetForwardVector()) > 0.5f)
	{
		BreakAnchor(false);
	}
	else
	{
		MantleEvent.Broadcast();
		GetWorld()->GetTimerManager().SetTimer(MantleTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
		{
			BreakAnchor();
		}), 0.8f, false);
	}
	bCanDoubleJump = true;
	bMantling = false;
}


void UPupMovementComponent::EdgeSlide(const float Scale, const float DeltaTime)
{
	if (FMath::Abs(Scale) < 0.5f)
	{
		return;
	}
	const float CurvedScale = Scale > 0.0f ? (Scale - 0.5f) * 2.0f : (Scale + 0.5f) * 2.0f;
	
	// TODO: Make this rate a UProperty?
	const float SlideRate = 100.0f;
	const float CapsuleRadius = 10.0f;
	
	const FVector SlideOffset = LedgeDirection * SlideRate * CurvedScale * DeltaTime;
	const FVector NewAnchorWorldLocation = UpdatedComponent->GetComponentLocation() + SlideOffset;
	const FVector RadiusCheckOffset = Scale > 0.0f ? LedgeDirection * CapsuleRadius : LedgeDirection * -CapsuleRadius;
	
	FHitResult LineTraceResult;
	if (AnchorTarget->LineTraceComponent(LineTraceResult,
		NewAnchorWorldLocation + RadiusCheckOffset,
		NewAnchorWorldLocation + RadiusCheckOffset + UpdatedComponent->GetForwardVector() * GrabRangeForward,
		FCollisionQueryParams::DefaultQueryParam))
	{
		AnchorWorldLocation = NewAnchorWorldLocation;
	}
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
				AnchorWorldLocation = LineTraceResult.Location + (CapsuleComponent->GetComponentLocation() - EyePosition);
				const FVector Difference = AnchorWorldLocation - Target->GetComponentLocation();
				AnchorRelativeLocation = FVector(
					FVector::DotProduct(Difference, Target->GetForwardVector()),
					FVector::DotProduct(Difference, Target->GetRightVector()),
					FVector::DotProduct(Difference, Target->GetUpVector()));
				AnchorRelativeYaw = Yaw - Target->GetComponentRotation().Yaw;
				AnchorTarget = Target;
				DesiredRotation.Yaw = Yaw;
				
				bMantling = true;
				bCanMantle = false;
				bIsWalking = false;
				SetMovementMode(EPupMovementMode::M_Anchored);
			}
		}
	}
}
