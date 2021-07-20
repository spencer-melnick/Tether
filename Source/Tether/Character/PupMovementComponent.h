// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PupMovementComponent.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EPupMovementMode : uint8
{
	M_None		UMETA(DisplayName = "None"),
	M_Walking	UMETA(DisplayName = "Walking"),
	M_Falling	UMETA(DisplayName = "Falling"),
	M_Anchored	UMETA(DisplayName = "Anchored"),
	M_Deflected UMETA(DisplayName = "Deflected")
};

UCLASS()
class TETHER_API UPupMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UPupMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

	bool SweepCapsule(const FVector Offset, FHitResult& OutHit) const;

	void SetDefaultMovementMode();
	
	void AddImpulse(const FVector Impulse);
	
	// Accessor Overrides
	
	virtual float GetMaxSpeed() const override;


	// Movement Logic

	bool FindFloor(const float Distance, FVector& Location);

	/** Sweeps for a valid floor beneath the character. If true, OutHitResult contains the sweep result */
	bool FindFloor(float SweepDistance, FHitResult& OutHitResult) const;

	/** Checks if the hit result was for a valid floor based on component settings and floor slope */
	bool IsValidFloorHit(const FHitResult& FloorHit) const;

	/** Moves the character to the floor, but does not update velocity */
	void SnapToFloor(const FHitResult& FloorHit);

	/** Attempts to jump, returns false if the jump can't be executed */
	bool Jump();
	
	// MovementComponent interface
	virtual float GetGravityZ() const override;

	void AnchorToLocation(const FVector& AnchorLocationIn);

	void BreakAnchor(const bool bForceBreak = false);
	
private:

	/** Perform one single movement step, with potential substeps */
	void StepMovement(float DeltaTime);

	/** Perform a single movement substep, returning the amount of time actually simulated in the substep */
	float SubstepMovement(float DeltaTime);

	void Land();

	void Fall();
	
	void TickGravity(const float DeltaTime);
	
	void HandleInputAxis();
	
	FRotator GetNewRotation(const float DeltaTime) const;
	
	FVector GetNewVelocity(const float DeltaTime);

	FVector ApplyFriction(const FVector& VelocityIn, const float DeltaTime) const;

	void RenderHitResult(const FHitResult& HitResult, const FColor Color = FColor::White) const;

	FVector ConsumeImpulse();
	
public:	
	// Properties

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	EPupMovementMode MovementMode = EPupMovementMode::M_Falling;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	float MaxSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	float MaxAcceleration = 200.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	float BreakingFriction = 2000.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	float RunningFriction = 1200.f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement")
	FVector DirectionVector;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement")
	bool bIsWalking = false;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement")
	float MovementSpeedAlpha = 0.0f;

	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Rotation")
	FRotator DesiredRotation = FRotator(0.f,0.f,0.f);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Rotation")
	float RotationSpeed = 360.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Rotation")
	bool bSlip = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Rotation")
	float SlipFactor = 0.8;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Rotation")
	float CameraYaw = 0.f;

	

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement | Jumping")
	bool bCanJump = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Jumping")
	float CoyoteTime = 0.5f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Jumping")
	float AirControlFactor = 0.2f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Jumping")
	float TerminalVelocity = -4000.f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement | Jumping")
	bool bGrounded = false;


	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Anchored")
	FVector AnchorLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Anchored")
	float SnapVelocity = 1000.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Anchored")
	float SnapRotationVelocity = 360.f;
	
private:

	UPROPERTY(Transient)
	UPrimitiveComponent* CurrentFloorComponent;

	FVector PendingImpulses;

	FTimerHandle CoyoteTimer;
};
