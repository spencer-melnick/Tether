// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "Camera/CameraComponent.h"
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

	virtual  void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	bool SweepCapsule(const FVector Offset, FHitResult& OutHit) const;

	UFUNCTION(BlueprintCallable)
	void SetDefaultMovementMode();

	UFUNCTION(BlueprintCallable)
	void AddImpulse(const FVector Impulse);
	
	// Accessor Overrides
	
	virtual float GetMaxSpeed() const override;


	// Movement Logic

	/** Sweeps for a valid floor beneath the character. If true, OutHitResult contains the sweep result */
	bool FindFloor(float SweepDistance, FHitResult& OutHitResult);

	/** Checks if the hit result was for a valid floor based on component settings and floor slope */
	bool IsValidFloorHit(const FHitResult& FloorHit) const;

	/** Moves the character to the floor, but does not update velocity */
	void SnapToFloor(const FHitResult& FloorHit);

	/** Attempts to jump, returns false if the jump can't be executed */
	UFUNCTION(BlueprintCallable)
	bool Jump();

	void StopJumping();
	
	// MovementComponent interface
	virtual float GetGravityZ() const override;

	UFUNCTION(BlueprintCallable)
	void AnchorToLocation(const FVector& AnchorLocationIn);

	UFUNCTION(BlueprintCallable)
	bool SetMovementMode(const EPupMovementMode& NewMovementMode);

	UFUNCTION(BlueprintCallable)
	void Deflect(const FVector& DeflectionVelocity);
	
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

	FVector ClampToPlaneMaxSize(const FVector& VectorIn, const FVector& Normal, const float MaxSize) const;
	
	FRotator GetNewRotation(const float DeltaTime) const;
	
	FVector GetNewVelocity(const float DeltaTime);

	FVector HoldJump(const float DeltaTime);

	FVector ApplyFriction(const FVector& VelocityIn, const float DeltaTime) const;

	FVector ApplySlidingFriction(const FVector& VelocityIn, const float DeltaTime, const float Friction) const;
	
	void RenderHitResult(const FHitResult& HitResult, const FColor Color = FColor::White) const;

	void RegainControl();
	
	FVector ConsumeImpulse();
	
public:	
	// Properties

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement")
	EPupMovementMode MovementMode = EPupMovementMode::M_Falling;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	float MaxSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	float MaxAcceleration = 200.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	float BreakingFriction = 2000.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	float RunningFriction = 1200.f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement")
	FVector DirectionVector;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement")
	bool bIsWalking = false;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement")
	float MovementSpeedAlpha = 0.0f;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient, Category = "Movement | Rotation")
	FRotator DesiredRotation = FRotator(0.f,0.f,0.f);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Rotation")
	float RotationSpeed = 360.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Rotation")
	bool bSlip = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Rotation", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float SlipFactor = 0.8;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement | Rotation")
	float CameraYaw = 0.f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement | Rotation")
	float TurningDirection = 0.0f;

	

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement | Jumping")
	bool bCanJump = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Jumping")
	float JumpInitialVelocity = 300.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Jumping")
	float ApexVelocity = 500.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement | Jumping")
	float HoldJumpAcceleration = 400.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Jumping", meta = (ClampMin = 0.0f))
	float MaxJumpTime = 0.5f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Jumping", meta = (ClampMin = 0.0f))
	float CoyoteTime = 0.5f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Jumping", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float AirControlFactor = 0.2f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Jumping")
	float TerminalVelocity = -4000.f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement | Jumping")
	bool bGrounded = false;

	bool bJumping = false;


	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient, Category = "Movement | Anchored")
	FVector AnchorLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Anchored")
	float SnapVelocity = 1000.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Anchored")
	float SnapRotationVelocity = 360.f;


	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement | Planar")
	FVector FloorNormal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Planar")
	float MaxIncline = 60.f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement | Planar")
	float MaxInclineZComponent = 0.5f;


	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Deflections")
	float DeflectionFriction = 800.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Deflections", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float DeflectionControlInfluence = 0.2f;

	/// If the player can regain control of the character early, by cancelling the velocity in the direction
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement | Deflections")
	bool bCanRegainControl = true;
	
private:

	UPROPERTY(Transient)
	UPrimitiveComponent* CurrentFloorComponent;
	
	UPROPERTY(Transient)
	FVector PendingImpulses;

	FVector DeflectDirection;
	
	float JumpAppliedVelocity;

	FTimerHandle CoyoteTimer;

	FTimerHandle JumpTimer;
};
