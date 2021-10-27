#include "PupMovementComponent.h"

#include "AITypes.h"
#include "DrawDebugHelpers.h"
#include "../TetherCharacter.h"
#include "Components/CapsuleComponent.h"

void UPupMovementComponent::SetDefaultMovementMode()
{
	ConsumeImpulse();
	FHitResult FloorResult;
	const float EffectiveSnapDistance = FMath::Max(FloorSnapDistance, Velocity.Z);
	if ( (FindFloor(EffectiveSnapDistance, FloorResult, 1) && Velocity.Z <= 0.0f) || bGrounded)
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
	bWallSliding = false;
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
	if (bWallSliding)
	{
		WallJump();
		float PlayerHeight = 0.0f;

		ATetherCharacter* Character = Cast<ATetherCharacter>(GetPawnOwner());
		UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		
		if (IsValid(Character) && Capsule)
		{
			PlayerHeight = Capsule->GetScaledCapsuleHalfHeight() + Capsule->GetScaledCapsuleRadius();
		}
		JumpEvent.Broadcast(BasisPositionLastTick - PlayerHeight * UpdatedComponent->GetUpVector(), true);

		
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
	if (bCanJump)
	{
		bAttachedToBasis = false;
		AddImpulse(UpdatedComponent->GetUpVector() * (JumpInitialVelocity - Velocity.Z) + BasisRelativeVelocity);
		
		JumpAppliedVelocity += JumpInitialVelocity;
		bCanJump = false;
		bJumping = true;

		ATetherCharacter* Character = Cast<ATetherCharacter>(GetPawnOwner());
		UCapsuleComponent* Capsule = Character->GetCapsuleComponent();

		float PlayerHeight = 0.0f;
		if (IsValid(Character) && Capsule)
		{
			PlayerHeight = Capsule->GetScaledCapsuleHalfHeight() + Capsule->GetScaledCapsuleRadius();
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
		UpdatedComponent->SetWorldRotation(DesiredRotation);

		if (bIsWalking)
		{
			const float ImpulseStrength = MaxSpeed - Speed;
			// Apply extra directional velocity
			AddImpulse(FMath::Min(MaxAcceleration * InputFactor * DoubleJumpAccelerationFactor, ImpulseStrength) * UpdatedComponent->GetForwardVector());
		}
		
		JumpAppliedVelocity += JumpInitialVelocity;
		bCanDoubleJump = false;
		bJumping = true;
		bWallJumpDisabledControl = false;

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


void UPupMovementComponent::WallJump()
{
	bWallSliding = false;
	JumpAppliedVelocity += JumpInitialVelocity;
	bCanJump = false;
	bJumping = true;
	bCanScramble = false;
		
	AddImpulse(MaxSpeed * WallNormal + (JumpInitialVelocity - Velocity.Z) * UpdatedComponent->GetUpVector() );
	DesiredRotation.Yaw += 180.0f;
	UpdatedComponent->AddWorldRotation(FRotator(0.0f, 180.0f, 0.0f));
	
	bWallJumpDisabledControl = true;
	GetWorld()->GetTimerManager().SetTimer(MantleDebounceTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]
		{
			bWallJumpDisabledControl = false;
		}),	WallJumpDisableTime, false);
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
		bAttachedToBasis = false;
		BasisComponent = nullptr;
		
		SetMovementMode(EPupMovementMode::M_Deflected);
		DeflectDirection = DeflectionVelocity.GetSafeNormal();

		const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(DeflectDirection.Y, DeflectDirection.X));
		FRotator NewRotation = UpdatedComponent->GetComponentRotation();
		NewRotation.Yaw = DesiredYaw;
		DesiredRotation.Yaw = DesiredYaw;
		UpdatedComponent->SetWorldRotation(NewRotation);

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
	bWallSliding = false;
	BasisComponent = FloorComponent;
	bMantling = false;
	bCanMantle = true;
	bCanScramble = true;
	
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
	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(UpdatedComponent))
	{
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	bAttachedToBasis = false;
	BasisComponent = nullptr;
	
	const FVector RecoveryLocation = LastValidLocation + (RecoveryLevitationHeight * UpdatedComponent->GetUpVector());
	const float GravityDelta = GetGravityZ() * RecoveryTime;
	FVector RecoveryVelocity = (RecoveryLocation - UpdatedComponent->GetComponentLocation()) / RecoveryTime;
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
	if (MovementMode == EPupMovementMode::M_Recover)
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(UpdatedComponent))
		{
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}

		Velocity = FVector::ZeroVector;
		UpdatedComponent->SetWorldLocation(LastValidLocation + RecoveryLevitationHeight * UpdatedComponent->GetUpVector());
		BasisPositionLastTick = UpdatedComponent->GetComponentLocation();
		ClearImpulse();
		SetDefaultMovementMode();
	}
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

	if (UPrimitiveComponent* CapsuleComponent = Cast<UPrimitiveComponent>(UpdatedComponent))
	{
		const float CapsuleRadius = CapsuleComponent->GetCollisionShape().GetCapsuleRadius();
		const float CapsuleHalfHeight = CapsuleComponent->GetCollisionShape().GetCapsuleHalfHeight();

		const FVector RotatedLedgeDirection = BasisComponent->GetComponentRotation().RotateVector(LedgeDirection);
		
		const FVector SlideOffset = RotatedLedgeDirection * SlideRate * CurvedScale * DeltaTime;
		const FVector NewAnchorWorldLocation = UpdatedComponent->GetComponentLocation() + SlideOffset;
		const FVector RadiusCheckOffset = Scale > 0.0f ? RotatedLedgeDirection * CapsuleRadius : RotatedLedgeDirection * -CapsuleRadius;
		
		FHitResult LineTraceResult;
		if (BasisComponent && GetWorld()->LineTraceSingleByChannel(LineTraceResult,
			NewAnchorWorldLocation + RadiusCheckOffset,
			NewAnchorWorldLocation + RadiusCheckOffset + UpdatedComponent->GetForwardVector() * GrabRangeForward,
			ECC_Pawn))
		{
			FHitResult TopLineTraceResult;
			FVector MantleOffset = FVector::ZeroVector;
			TArray<USceneComponent*> ChildComponents;
			UpdatedComponent->GetChildrenComponents(false, ChildComponents);
			for (USceneComponent* Component : ChildComponents)
			{
				if (Component->ComponentHasTag(TEXT("MantleHandle")))
				{
					// TODO: Don't search for components this way
					MantleOffset = Component->GetRelativeLocation();
					break;
				}
			}
			MantleOffset.X += 5.0f;
			const float SinYaw = FMath::Sin(FMath::DegreesToRadians(DesiredRotation.Yaw));
			const float CosYaw = FMath::Cos(FMath::DegreesToRadians(DesiredRotation.Yaw));
			const FVector MantleOffsetWorldSpace = FVector(MantleOffset.X * CosYaw - MantleOffset.Y * SinYaw, MantleOffset.X * SinYaw + MantleOffset.Y * CosYaw, MantleOffset.Z);

			const FVector MantleLocation = UpdatedComponent->GetComponentLocation() + MantleOffsetWorldSpace;
			const FVector TraceStart = MantleLocation + SlideOffset + RadiusCheckOffset + FVector::UpVector * GrabRangeTop;
			const FVector TraceEnd = MantleLocation + SlideOffset + RadiusCheckOffset + FVector::DownVector * GrabRangeBottom;

			FCollisionQueryParams CollisionQueryParams = FCollisionQueryParams::DefaultQueryParam;
			CollisionQueryParams.AddIgnoredActors(IgnoredActors);
			CollisionQueryParams.bFindInitialOverlaps = true;
			CollisionQueryParams.bIgnoreTouches = false;
			CollisionQueryParams.bTraceComplex = false;

			GetWorld()->LineTraceSingleByChannel(TopLineTraceResult, TraceStart, TraceEnd, ECC_Pawn, CollisionQueryParams);
			if (TopLineTraceResult.bBlockingHit && !TopLineTraceResult.bStartPenetrating)
			{
				if (LineTraceResult.GetComponent() != BasisComponent)
				{
					BasisComponent = LineTraceResult.GetComponent();
					FVector NewBasisWorldLocation = UpdatedComponent->GetComponentLocation();
					NewBasisWorldLocation.Z = TopLineTraceResult.Location.Z - MantleOffsetWorldSpace.Z;
					UpdatedComponent->SetWorldLocation(NewBasisWorldLocation);
				}
				// UpdatedComponent->AddWorldOffset(SlideOffset);
				DesiredAnchorLocation += SlideOffset;
				TurningDirection = CurvedScale;
				// LedgeDirection = FVector::CrossProduct(WallNormal, TopLineTraceResult.Normal);
				return true;
			}
			RenderHitResult(TopLineTraceResult, FColor::Red);
		}
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
	
	FCollisionQueryParams CollisionQueryParams = FCollisionQueryParams::DefaultQueryParam;
	CollisionQueryParams.AddIgnoredActor(this->GetOwner());

	FHitResult LineTraceResult;
	if(GetWorld()->LineTraceSingleByChannel(LineTraceResult,
		EyePosition, EyePosition + CapsuleComponent->GetForwardVector() * GrabRangeForward,
		ECollisionChannel::ECC_Pawn, CollisionQueryParams,
		CapsuleComponent->GetCollisionResponseToChannels()))
	{
		if (LineTraceResult.GetComponent() && LineTraceResult.GetComponent()->CanCharacterStepUp(this->GetPawnOwner()))
		{
			UPrimitiveComponent* Target = LineTraceResult.GetComponent();
			const FVector EdgeWallNormal = LineTraceResult.Normal;

			// Where the ledge is in world space, as far as we know...
			// We look slightly into the wall to avoid missing the next sweep
			FVector MantleLocation = FVector::ZeroVector;
			Target->GetClosestPointOnCollision(UpdatedComponent->GetComponentLocation(), MantleLocation);
			MantleLocation -= EdgeWallNormal;
			MantleLocation.Z += CapsuleComponent->GetScaledCapsuleHalfHeight();

			CollisionQueryParams = FCollisionQueryParams::DefaultQueryParam;
			CollisionQueryParams.AddIgnoredActors(IgnoredActors);
			CollisionQueryParams.bFindInitialOverlaps = true;
			CollisionQueryParams.bIgnoreTouches = false;
			CollisionQueryParams.bTraceComplex = false;

			FHitResult TopLineTraceResult;
			GetWorld()->LineTraceSingleByChannel(TopLineTraceResult,
				MantleLocation + FVector::UpVector * GrabRangeTop,
				MantleLocation + FVector::DownVector * GrabRangeBottom,
				ECollisionChannel::ECC_Pawn, CollisionQueryParams);

			if (!TopLineTraceResult.bStartPenetrating && TopLineTraceResult.bBlockingHit)
			{
				const FVector TopWallNormal = TopLineTraceResult.Normal;
				LedgeDirection = FVector::CrossProduct(TopWallNormal, EdgeWallNormal).GetUnsafeNormal();

				// Check how close our ledge direction vector is to the 'right vector',
				// assuming that the 'forward vector' is the direction the player will rotate towards --
				// the opposite of the WallNormal 
				if (FMath::Abs(FVector::DotProduct(LedgeDirection, EdgeWallNormal.RotateAngleAxis(90.0f, FVector::UpVector))) >= LedgeDeviation)
				{
					// Verify that we can actually 'fit' along the ledge
					const FVector LeftSide = UpdatedComponent->GetComponentLocation() - LedgeDirection * CapsuleComponent->GetCollisionShape().GetCapsuleRadius() - FVector::DownVector;
					const FVector RightSide = UpdatedComponent->GetComponentLocation() + LedgeDirection * CapsuleComponent->GetCollisionShape().GetCapsuleRadius() - FVector::DownVector;
					const FVector Offset =  (MantleLocation - UpdatedComponent->GetComponentLocation()).GetSafeNormal2D() * (GrabRangeForward + CapsuleRadius);

					FHitResult SizeTraceLeft;
					FHitResult SizeTraceRight;
					
					if (Target->LineTraceComponent(SizeTraceLeft, LeftSide, LeftSide + Offset, CollisionQueryParams) &&
					Target->LineTraceComponent(SizeTraceRight, RightSide, RightSide + Offset, CollisionQueryParams))
					{
						FVector AnchorWorldLocation = FVector(MantleLocation.X, MantleLocation.Y, TopLineTraceResult.Location.Z);

						FVector MantleOffset = FVector::ZeroVector;
						TArray<USceneComponent*> ChildComponents;
						UpdatedComponent->GetChildrenComponents(false, ChildComponents);
						for (USceneComponent* Component : ChildComponents)
						{
							if (Component->ComponentHasTag(TEXT("MantleHandle")))
							{
								// TODO: Don't search for components this way
								MantleOffset = Component->GetRelativeLocation();
								break;
							}
						}
						
						const float SinYaw = FMath::Sin(FMath::DegreesToRadians(DesiredRotation.Yaw));
						const float CosYaw = FMath::Cos(FMath::DegreesToRadians(DesiredRotation.Yaw));
						const FVector MantleOffsetWorldSpace = FVector(MantleOffset.X * CosYaw - MantleOffset.Y * SinYaw, MantleOffset.X * SinYaw + MantleOffset.Y * CosYaw, MantleOffset.Z);

						AnchorWorldLocation -= MantleOffsetWorldSpace;
						const FVector Difference = AnchorWorldLocation - Target->GetComponentLocation();
						
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
						bWallSliding = false;
						bAttachedToBasis = true;
						bWallJumpDisabledControl = false;

						BasisComponent = Target;
						SetMovementMode(EPupMovementMode::M_Anchored);

						const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-LineTraceResult.ImpactNormal.Y, -LineTraceResult.ImpactNormal.X));
						// UpdatedComponent->SetWorldLocation(AnchorWorldLocation);
						DesiredAnchorLocation = AnchorWorldLocation;
						UpdatedComponent->SetWorldRotation(FRotator(0.0f, Yaw, 0.0f));
						DesiredRotation = FRotator(0.0f, Yaw, 0.0f);
					}
				}
			}
		}
	}
	// The line trace originated at the eye, so we can look there, plus 1.0f in case the player is
	// very close to the wall
	if (LineTraceResult.bBlockingHit && LineTraceResult.Distance <= CapsuleRadius + 1.0f)
	{
		HitWall(LineTraceResult);
	}
}


void UPupMovementComponent::HitWall(const FHitResult& HitResult)
{
	if (!bGrounded && MovementMode == EPupMovementMode::M_Falling)
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(HitResult.Component) )
		{
			FHitResult LineTrace;
			
			bAttachedToBasis = true;
			BasisComponent = PrimitiveComponent;
			bWallSliding = true;
			
			if (PrimitiveComponent->LineTraceComponent(LineTrace, UpdatedComponent->GetComponentLocation(),
				UpdatedComponent->GetComponentLocation() + DirectionVector * 100.0f,
				FCollisionQueryParams::DefaultQueryParam))
			{
				const FVector PlanarNormal = LineTrace.Normal.GetSafeNormal2D();
				const float WallYaw = FMath::RadiansToDegrees( FMath::Atan2(-PlanarNormal.Y, -PlanarNormal.X) );
				DesiredRotation.Yaw = WallYaw;
				// UpdatedComponent->SetWorldRotation(DesiredRotation);
				WallNormal = PlanarNormal;
			}
		}
	}
}