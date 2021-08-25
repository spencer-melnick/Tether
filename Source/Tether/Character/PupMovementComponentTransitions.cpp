#include "PupMovementComponent.h"

#include "AITypes.h"
#include "DrawDebugHelpers.h"
#include "TetherCharacter.h"
#include "Components/CapsuleComponent.h"


bool UPupMovementComponent::Jump()
{
	if (bCanJump)
	{
		AddImpulse(UpdatedComponent->GetUpVector() * JumpInitialVelocity);
		
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
		JumpEvent.Broadcast(LastBasisPosition - PlayerHeight * UpdatedComponent->GetUpVector());
		
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
	if (FVector::DotProduct(DeflectDirection, Velocity) <= KINDA_SMALL_NUMBER)
	{
		DeflectDirection = FVector::ZeroVector;
		if (MovementMode == EPupMovementMode::M_Deflected)
		{
			SetDefaultMovementMode();
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


void UPupMovementComponent::AnchorToComponent(UPrimitiveComponent* AnchorTargetComponent)
{
	if (!AnchorTargetComponent)
	{
		return;
	}
	
	FHitResult AnchorTestHit;
	UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(UpdatedComponent);
	FVector CloseLocation = FVector::ZeroVector;
	AnchorTargetComponent->GetClosestPointOnCollision(UpdatedComponent->GetComponentLocation(), CloseLocation);
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	AnchorTargetComponent->SweepComponent(AnchorTestHit, ComponentLocation,
		CloseLocation,
		UpdatedComponent->GetComponentQuat(),
		PrimitiveComponent->GetCollisionShape());

	AnchorTarget = AnchorTargetComponent;
	AnchorWorldLocation = AnchorTestHit.Location + AnchorTestHit.ImpactNormal * 2.0f;
	const FVector Distance = AnchorWorldLocation - AnchorTarget->GetComponentLocation();
	AnchorRelativeLocation =  FVector(FVector::DotProduct(Distance, AnchorTarget->GetForwardVector()),
		FVector::DotProduct(Distance, AnchorTarget->GetRightVector()),
		FVector::DotProduct(Distance, AnchorTarget->GetUpVector()));
	const FVector RefLocation = AnchorWorldLocation + AnchorTestHit.ImpactNormal * -2.0f;
	const float NewAnchorYaw = FMath::RadiansToDegrees(FMath::Atan2(
		RefLocation.Y - ComponentLocation.Y,
		RefLocation.X - ComponentLocation.X));
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
}


void UPupMovementComponent::Land(const FVector& FloorLocation, const float ImpactVelocity)
{
	SetMovementMode(EPupMovementMode::M_Walking);
	bCanJump = true;
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