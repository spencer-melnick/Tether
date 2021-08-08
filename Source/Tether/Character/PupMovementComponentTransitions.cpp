#include "PupMovementComponent.h"

#include "AITypes.h"
#include "DrawDebugHelpers.h"
#include "TetherCharacter.h"


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