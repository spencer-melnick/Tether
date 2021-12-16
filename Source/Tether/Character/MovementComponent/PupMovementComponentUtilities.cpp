#include "PupMovementComponent.h"

#include "DrawDebugHelpers.h"
#include "../TetherCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Tether/Tether.h"


// Utilities
namespace  PupMovementCVars
{
	static float FloorPadding = 0.01f;
	static FAutoConsoleVariableRef CVarFloorPadding(
		TEXT("PupMovement.FloorPadding"),
		FloorPadding,
		TEXT("The minimum distance when snapping to the floor."),
		ECVF_Default);
}


bool UPupMovementComponent::FindFloor(const float SweepDistance, FHitResult& OutHitResult, const int NumTries)
{
	const FVector SweepOffset = FVector::DownVector * SweepDistance;

	if (MovementMode == EPupMovementMode::M_Anchored)
	{
		InvalidFloorComponents.Add(BasisComponent);
	}
	for (int i = 0; i < NumTries; i++)
	{
		FHitResult IterativeHitResult;
		if (SweepCapsule(FVector(0.0f, 0.0f, 10.0f),SweepOffset, IterativeHitResult, false))
		{
			OutHitResult = IterativeHitResult;
			// RenderHitResult(IterativeHitResult, FColor::White, true);
			if (IsValidFloorHit(IterativeHitResult))
			{
				FloorNormal = IterativeHitResult.ImpactNormal;
				return true;
			}
			if (OutHitResult.bStartPenetrating)
			{
				// ResolvePenetration(GetPenetrationAdjustment(OutHitResult), OutHitResult, UpdatedComponent->GetComponentQuat());
				InvalidFloorComponents.Add(OutHitResult.GetComponent());
			}
		}
	}
	InvalidFloorComponents.Empty();
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
		// so, the most realiable way of getting the floor's normal is with a line trace.
		FHitResult NormalLineTrace;
		FloorComponent->LineTraceComponent(
			NormalLineTrace,
			FloorHit.ImpactPoint + FVector(0.0f, 0.0f, 250.0f),
			FloorHit.ImpactPoint + FVector(0.0f, 0.0f, -250.0f),
			FCollisionQueryParams::DefaultQueryParam);
		// RenderHitResult(NormalLineTrace, FColor::Red);
		if (NormalLineTrace.ImpactNormal.Z >= MaxInclineZComponent && !FloorHit.bStartPenetrating)
		{
			return true;
		}
	}
	return false;
}


void UPupMovementComponent::SnapToFloor(const FHitResult& FloorHit)
{
	FHitResult DiscardHit;
	if (CheckFloorValidWithinRange(MinimumSafeRadius, FloorHit))
	{
		LastValidLocation = FloorHit.Location;
	}
	FHitResult DispatchHitResult = FHitResult(FloorHit);
	DispatchHitResult.Component = Cast<UPrimitiveComponent>(UpdatedComponent);
	DispatchHitResult.Actor = GetPawnOwner();

	if (DispatchHitResult.IsValidBlockingHit())
	{
		FloorHit.GetComponent()->DispatchBlockingHit(*FloorHit.GetActor(), DispatchHitResult);
	}
	
	SafeMoveUpdatedComponent(
		FloorHit.Location - UpdatedComponent->GetComponentLocation() + FloorHit.Normal * PupMovementCVars::FloorPadding,
		UpdatedComponent->GetComponentQuat(), true, DiscardHit, ETeleportType::None);
}


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
	return SweepCapsule(FVector::ZeroVector, Offset, OutHit, bIgnoreInitialOverlap);
}

bool UPupMovementComponent::SweepCapsule(const FVector InitialOffset, const FVector Offset, FHitResult& OutHit,
	const bool bIgnoreInitialOverlap) const
{
	ATetherCharacter* Character = Cast<ATetherCharacter>(GetPawnOwner());
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	UWorld* World = GetWorld();

	if (IsValid(Character) && IsValid(Capsule) && World)
	{
		const FVector Start = Capsule->GetComponentLocation() + InitialOffset;
		const FVector End = Capsule->GetComponentLocation() + Offset;

		FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
		QueryParams.bIgnoreTouches = true;
		QueryParams.bFindInitialOverlaps = !bIgnoreInitialOverlap;

		QueryParams.AddIgnoredActor(Character);
		QueryParams.AddIgnoredActors(IgnoredActors);
		QueryParams.AddIgnoredComponents(InvalidFloorComponents);

		const FCollisionResponseParams ResponseParams = FCollisionResponseParams::DefaultResponseParam;

		return World->SweepSingleByChannel(OutHit, Start, End, Capsule->GetComponentQuat(), ECC_Pawn,
			Capsule->GetCollisionShape(), QueryParams, ResponseParams);
	}
	return false;
}


void UPupMovementComponent::RenderHitResult(const FHitResult& HitResult, const FColor Color, const bool bPersistent) const
{
	/*
	if (HitResult.GetComponent())
	{
		DrawDebugDirectionalArrow(GetWorld(), HitResult.ImpactPoint + HitResult.ImpactNormal * 10.0f, HitResult.ImpactPoint, 5.0f, FColor::Black, false, bPersistent ? 0.015f : 2.0f, -1, 0.5f);

		DrawDebugLine(GetWorld(),
			HitResult.TraceStart,
			HitResult.TraceEnd, Color, false,
			bPersistent ? 0.015f : 2.0f, -1, 0.5f);
		DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 15.0f, Color, false, bPersistent ? 0.015f : 2.0f, -1);
		DrawDebugString(GetWorld(), HitResult.ImpactPoint + HitResult.ImpactNormal * 100.0f,
			HitResult.GetComponent()->GetName(), nullptr, Color, 0.015f, false, 1.0f);
	}
	else
	{
		DrawDebugLine(GetWorld(),
			HitResult.TraceStart,
			HitResult.TraceEnd, Color, false,
			bPersistent ? 0.015f : 2.0f, -1, 0.5f);
		DrawDebugString(GetWorld(), HitResult.ImpactPoint + HitResult.ImpactNormal * 100.0f,
		TEXT("X"), nullptr, Color, bPersistent ? 0.015f : 2.0f, false, 1.0f);
	}*/
}

void UPupMovementComponent::AddRootMotionTransform(const FTransform& RootMotionTransform)
{
	PendingRootMotionTransforms += RootMotionTransform;
}

void UPupMovementComponent::HandleRootMotion()
{
	if (MovementMode == EPupMovementMode::M_Anchored)
	{
		const FVector Translation = PendingRootMotionTransforms.GetTranslation();
		UpdatedComponent->AddWorldOffset(Translation);
		LocalBasisPosition += Translation;
		DesiredAnchorLocation += Translation;
	}
	else
	{
		FHitResult EmptyResult;
		SafeMoveUpdatedComponent(PendingRootMotionTransforms.GetTranslation(),
			UpdatedComponent->GetComponentQuat(),
			true, EmptyResult);
	}
	PendingRootMotionTransforms = FTransform::Identity;
}


bool UPupMovementComponent::CheckFloorValidWithinRange(const float Range, const FHitResult& HitResult)
{
	bool bResult = false;

	if (HitResult.GetComponent() && HitResult.GetComponent()->Mobility != EComponentMobility::Movable)
	{
		const FVector DirectionFromCenter = (HitResult.ImpactPoint - HitResult.Component->GetComponentLocation()).GetSafeNormal();
		const FVector NewSweepLocation = HitResult.ImpactPoint + DirectionFromCenter * Range;
		
		FHitResult NewHitResult;
		FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
		bResult = HitResult.GetComponent()->LineTraceComponent(NewHitResult, NewSweepLocation, NewSweepLocation + FVector::UpVector * -10.0f, QueryParams);
	}
	return bResult;
}


// ReSharper disable once CppMemberFunctionMayBeConst
// This function may be changed to modify the movementcomponent directly, and probably should not be const forever

/* This function is designed to resolve edge cases where another object pushes into the player */
void UPupMovementComponent::HandleExternalOverlaps(const float DeltaTime)
{
	if (MovementMode == EPupMovementMode::M_Recover)
	{
		return;
	}
	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(UpdatedComponent))
	{
		FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
		QueryParams.AddIgnoredActor(GetOwner());
		QueryParams.AddIgnoredActors(IgnoredActors);
		/* if (GetWorld()->OverlapBlockingTestByChannel(
			PrimitiveComponent->GetComponentLocation(),
			PrimitiveComponent->GetComponentQuat(),
			ECollisionChannel::ECC_Pawn,
			PrimitiveComponent->GetCollisionShape(),
			QueryParams)) */
		TArray<FOverlapResult> OverlapResults;
		if (GetWorld()->OverlapMultiByChannel(
			OverlapResults, PrimitiveComponent->GetComponentLocation(), PrimitiveComponent->GetComponentQuat(),
			ECC_Pawn, PrimitiveComponent->GetCollisionShape(),
			QueryParams))
		{
			FHitResult EscapeHit;
			FVector EndLocation = OverlapResults[0].Component->GetComponentLocation();
			FVector StartLocation =  UpdatedComponent->GetComponentLocation();
			/* if (!Velocity.IsNearlyZero())
			{
				EndLocation = StartLocation;
				StartLocation = EndLocation + Velocity.GetClampedToMaxSize(100.0f);
			} */
			const FQuat Rotator = UpdatedComponent->GetComponentQuat();
			FCollisionQueryParams CollisionQueryParams = FCollisionQueryParams::DefaultQueryParam;
			CollisionQueryParams.AddIgnoredActor(GetOwner());
			CollisionQueryParams.AddIgnoredComponent(BasisComponent);

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

void UPupMovementComponent::ResetState(FPupMovementComponentState* State)
{
	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(UpdatedComponent))
	{
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	
	BasisRelativeVelocity = FVector::ZeroVector;
	BasisPositionLastTick = FVector::ZeroVector;
	BasisRotationLastTick = FRotator::ZeroRotator;

	ConsumeAdjustments();
	ConsumeImpulse();
	
	if (State)
	{
		SetMovementMode(State->GetMovementMode());
		UpdatedComponent->SetWorldTransform(State->GetTransform());
		Velocity = State->GetVelocity();
	}
	GetWorld()->GetTimerManager().ClearTimer(RecoveryTimerHandle);

	const float TimerRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(RecoveryTimerHandle);
	const bool bIsPaused = GetWorld()->GetTimerManager().IsTimerPaused(RecoveryTimerHandle);
	UE_LOG(LogTetherGame, Display, TEXT("Time left until recovery: %f. Timer paused? %s"), &TimerRemaining, bIsPaused ? TEXT("true") : TEXT("false"));
	bAttachedToBasis = false;
	BasisComponent = nullptr;
	
	DesiredRotation = UpdatedComponent->GetComponentRotation();
}


void UPupMovementComponent::PauseTimers()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		if (TimerManager.TimerExists(RecoveryTimerHandle) && TimerManager.IsTimerActive(RecoveryTimerHandle))
		{
			TimerManager.PauseTimer(RecoveryTimerHandle);
		}
	}
}


void UPupMovementComponent::UnPauseTimers()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		if (TimerManager.TimerExists(RecoveryTimerHandle))
		{
			TimerManager.UnPauseTimer(RecoveryTimerHandle);
		}
	}
}


bool UPupMovementComponent::MatchModes(const EPupMovementMode& Subject, std::initializer_list<EPupMovementMode> CheckModes)
{
	for (EPupMovementMode MovementMode : CheckModes)
	{
		if (MovementMode == Subject)
		{
			return true;
		}
	}
	return false;
}


FVector UPupMovementComponent::GetLocationRelativeToComponent(const FVector& WorldLocation, USceneComponent* Component) const
{
	if (!Component)
	{
		return FVector::ZeroVector;
	}
	const FVector ComponentLocation = Component->GetComponentLocation();
	const FVector Distance = WorldLocation - ComponentLocation;
		
	return FVector(
		FVector::DotProduct(Distance, Component->GetForwardVector()),
		FVector::DotProduct(Distance, Component->GetRightVector()),
		FVector::DotProduct(Distance, Component->GetUpVector()));
}




FPupMovementComponentState::FPupMovementComponentState()
	:MovementMode(EPupMovementMode::M_None), Transform(FTransform::Identity), Velocity(FVector::ZeroVector)
{}


FPupMovementComponentState::FPupMovementComponentState(const UPupMovementComponent* Src)
	:MovementMode(Src->GetMovementMode()), Transform(Src->UpdatedComponent->GetComponentTransform()), Velocity(Src->Velocity)
{}
