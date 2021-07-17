// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "PupMovementComponent.h"

#include "DrawDebugHelpers.h"
#include "TetherCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"


namespace PupMovementCVars
{
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

void UPupMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* TickFunction)
{
	HandleInputAxis();
	UpdatedComponent->SetWorldRotation(GetNewRotation(DeltaTime));

	FVector LocationOnFloor;
	bGrounded = FindFloor(-10.0f, LocationOnFloor);
	if (!bGrounded)
	{
		TickGravity(DeltaTime);
	}
	else
	{
		Velocity.Z = FMath::Max(Velocity.Z, 0.0f);
	}
	if(bIsWalking)
	{
		Velocity = GetNewVelocity(DeltaTime);
	}
	ApplyFriction(DeltaTime);


	float TimeRemaining = DeltaTime;
	int32 NumSubsteps = 0;
	while (TimeRemaining > SMALL_NUMBER && NumSubsteps < PupMovementCVars::MaxSubsteps)
	{
		NumSubsteps++;
		TimeRemaining -= TickMovement(TimeRemaining);
	}

	// We're probably stuck in something, try to cancel player movement and do one last tick (for falling)
	if (TimeRemaining > SMALL_NUMBER)
	{
		Velocity.X = 0.f;
		Velocity.Y = 0.f;
		TickMovement(TimeRemaining);
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
		
		const FCollisionResponseParams ResponseParams = FCollisionResponseParams::DefaultResponseParam;
		
		return World->SweepSingleByChannel(OutHit, Start, End, Capsule->GetComponentQuat(), ECC_Pawn,
			Capsule->GetCollisionShape(), QueryParams, ResponseParams);
	}
	return false;
}


bool UPupMovementComponent::SweepCapsuleMultiple(const FVector DesiredLocation, FHitResult& OutHit) const
{
	ATetherCharacter* Character = Cast<ATetherCharacter>(GetPawnOwner());
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	UWorld* World = GetWorld();
	
	if (IsValid(Character) && IsValid(Capsule) && World)
	{
		const FVector Start = Capsule->GetComponentLocation();
		const FVector End = DesiredLocation;
		
		FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
		QueryParams.AddIgnoredActor(Character);

		if(CurrentFloorComponent && bGrounded)
		{
			QueryParams.AddIgnoredComponent(CurrentFloorComponent);
		}
		
		const FCollisionResponseParams ResponseParams = FCollisionResponseParams::DefaultResponseParam;

		return World->SweepSingleByChannel(OutHit, Start, End, Capsule->GetComponentQuat(), ECC_Pawn,
			Capsule->GetCollisionShape(), QueryParams, ResponseParams);
	}
	return false;
}


void UPupMovementComponent::AddImpulse(const FVector Impulse)
{
	Velocity += Impulse;
}


float UPupMovementComponent::GetMaxSpeed() const
{
	return MaxSpeed;
}


bool UPupMovementComponent::FindFloor(const float Distance, FVector& Location)
{
	FHitResult FloorHitResult;
	const FVector Offset = FVector(0.0f, 0.0f, FMath::Min(Distance, 0.0f));
	if (SweepCapsule(Offset, FloorHitResult))
	{
		UPrimitiveComponent* FloorComponent = FloorHitResult.GetComponent();
		CurrentFloorComponent = FloorComponent;
		Location = FloorHitResult.Location;

		if (FloorHitResult.bStartPenetrating)
		{
			return false;
		}
		
		RenderHitResult(FloorHitResult, FColor::Red);
		return FloorComponent && FloorComponent->CanCharacterStepUp(GetPawnOwner());
	}
	CurrentFloorComponent = nullptr;
	return false;
}

float UPupMovementComponent::TickMovement(float DeltaTime)
{
	const float MovementTime = PerformMovement(Velocity * DeltaTime);
	UpdateComponentVelocity();

	return MovementTime * DeltaTime;
}

float UPupMovementComponent::PerformMovement(const FVector& DeltaLocation)
{
	FHitResult HitResult;

	const float MovementDistance = DeltaLocation.Size();
	const float TraceDistance = FMath::Max(MovementDistance, PupMovementCVars::MinimumTraceDistance);

	if (FMath::IsNearlyZero(MovementDistance))
	{
		return 1.f;
	}

	const FVector StartingLocation = UpdatedComponent->GetComponentLocation();
	const FVector MovementDirection = DeltaLocation / MovementDistance;
	const FVector NewLocation = StartingLocation + MovementDirection * TraceDistance;
	
	if (!SweepCapsuleMultiple(NewLocation, HitResult))
	{
		UpdatedComponent->SetWorldLocation(StartingLocation + DeltaLocation);
		return 1.f;
	}

	FVector Movement;

	// Push the character some distance back from the impact to avoid getting stuck
	const float InflationDistance = PupMovementCVars::MinimumTraceDistance;
	float ActualMoveDistance;
	
	if (HitResult.bStartPenetrating)
	{
		ActualMoveDistance = 0.f;
		Movement = HitResult.Normal * HitResult.PenetrationDepth; 
	}
	else
	{
		ActualMoveDistance = FMath::Min(HitResult.Distance - InflationDistance, MovementDistance);
		Movement = MovementDirection * ActualMoveDistance;
		Velocity -= FVector::DotProduct(Velocity, HitResult.ImpactNormal) * HitResult.ImpactNormal;
	}
	
	UpdatedComponent->SetWorldLocation(StartingLocation + Movement);
	RenderHitResult(HitResult, FColor::Blue);

	return ActualMoveDistance / MovementDistance;
}


void UPupMovementComponent::TickGravity(float DeltaTime)
{
	if (UWorld* World = GetWorld())
	{
		Velocity.Z = FMath::Max(Velocity.Z + World->GetGravityZ() * DeltaTime, TerminalVelocity);
	}
}


void UPupMovementComponent::HandleInputAxis()
{
	const FVector InputVector = ConsumeInputVector();
	if(InputVector != FVector::ZeroVector)
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
	}
}


FRotator UPupMovementComponent::GetNewRotation(float DeltaTime)
{
	float RotationRate = RotationSpeed;
	if (bSlip)
	{
		RotationRate *= 1 - (1 - SlipFactor) * (Velocity.Size2D() / MaxSpeed);
	}
	return FMath::RInterpConstantTo(UpdatedComponent->GetComponentRotation(), DesiredRotation, DeltaTime, RotationRate);
}


FVector UPupMovementComponent::GetNewVelocity(float DeltaTime)
{
	FVector Acceleration = MaxAcceleration * DirectionVector * DeltaTime;
	if (!bGrounded)
	{
		Acceleration *= AirControlFactor;
	}
	return (Velocity + Acceleration).GetClampedToMaxSize2D(MaxSpeed);
}


void UPupMovementComponent::ApplyFriction(float DeltaTime)
{
	if(bGrounded)
	{
		if(bIsWalking)
		{
			const FVector DirectionVectorNormalized = DirectionVector.GetSafeNormal();
			FVector VelocityInCurrentDirection = FVector::DotProduct(Velocity, DirectionVectorNormalized) * DirectionVectorNormalized;
			VelocityInCurrentDirection.Z += Velocity.Z;
			Velocity = FMath::VInterpConstantTo(Velocity, VelocityInCurrentDirection, DeltaTime, RunningFriction);
		}
		else
		{
			const FVector NewVelocity = FMath::VInterpConstantTo(Velocity, FVector::ZeroVector, DeltaTime, BreakingFriction);
			Velocity.X = NewVelocity.X;
			Velocity.Y = NewVelocity.Y;
		}
	}
}


void UPupMovementComponent::RenderHitResult(const FHitResult& HitResult, const FColor Color) const
{
	if (HitResult.GetComponent())
	{
		DrawDebugLine(GetWorld(), HitResult.ImpactPoint, HitResult.ImpactPoint + HitResult.ImpactNormal * 100.0f, Color, false, 1.0f, 1.0f, 2.0f);
		DrawDebugString(GetWorld(), HitResult.ImpactPoint + HitResult.ImpactNormal * 100.0f, HitResult.GetComponent()->GetName(), 0, Color, 0.02f, false, 1.0f);
	}
}
