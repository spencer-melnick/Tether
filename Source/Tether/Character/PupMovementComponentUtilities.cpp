#include "PupMovementComponent.h"

#include "AITypes.h"
#include "DrawDebugHelpers.h"
#include "TetherCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"


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
	if (bDrawMovementDebug)
	{
		if (HitResult.GetComponent())
		{
			DrawDebugLine(GetWorld(),
				HitResult.ImpactPoint,
				HitResult.ImpactPoint + HitResult.ImpactNormal * 100.0f, Color, false,
				bPersistent ? 0.015f : 2.0f, -1, 2.0f);
			DrawDebugString(GetWorld(), HitResult.ImpactPoint + HitResult.ImpactNormal * 100.0f,
				HitResult.GetComponent()->GetName(), nullptr, Color, 0.015f, false, 1.0f);
		}
	}
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
		if (GetWorld()->OverlapBlockingTestByChannel(
			PrimitiveComponent->GetComponentLocation(),
			PrimitiveComponent->GetComponentQuat(),
			ECollisionChannel::ECC_WorldDynamic,
			PrimitiveComponent->GetCollisionShape(),
			QueryParams))
		{
			FHitResult EscapeHit;
			FVector EndLocation = UpdatedComponent->GetComponentLocation() + FVector::UpVector * 100.0f;
			FVector StartLocation =  UpdatedComponent->GetComponentLocation();
			if (!Velocity.IsNearlyZero())
			{
				EndLocation = StartLocation;
				StartLocation = EndLocation + Velocity.GetClampedToMaxSize(100.0f);
			}
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
