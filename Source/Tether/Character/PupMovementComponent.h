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
	M_Deflected UMETA(DisplayName = "Deflected"),
	M_Recover	UMETA(DisplayName = "Recover")
};


UCLASS()
class TETHER_API UPupMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UPupMovementComponent();

	
	// Overrides
	virtual  void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual float GetMaxSpeed() const override;

	virtual float GetGravityZ() const override;

	virtual bool ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation) override;
	
	// Movement mode transitions
	UFUNCTION(BlueprintCallable)
	bool SetMovementMode(const EPupMovementMode& NewMovementMode);

	EPupMovementMode GetMovementMode() const;
	
	/**
	* Set the player's movement mode to whatever mode is appropriate, given their position and velocity.
	* For instance, a player in the air will be set to 'Falling'.
	* This should be called when an object does not have access to the
	* necessary context to determine the correct movement mode.
	*/
	UFUNCTION(BlueprintCallable)
	void SetDefaultMovementMode();


	/** Grab onto a specific location, preventing the player from falling or moving */
	UFUNCTION(BlueprintCallable)
	void AnchorToComponent(UPrimitiveComponent* AnchorTargetComponent);

	/** Have the player let go of an anchor point, optionally forcing them */
	UFUNCTION(BlueprintCallable)
	void BreakAnchor(const bool bForceBreak = false);

	/** Launch the player in a direction, causing them to lose control temporarily */
	UFUNCTION(BlueprintCallable)
	void Deflect(const FVector& DeflectionVelocity, const float DeflectTime);
	
	/** Try to give the player back control if they have been deflected by some object */
	void TryRegainControl();
	
	UFUNCTION(BlueprintCallable)
	/** Attempt to jump, returns false if the jump can't be executed */
	bool Jump();

	/** Force jump to end, automatically called after MaxJumpTime seconds have elapsed **/
	void StopJumping();

	/**
	* Add velocity directly to the player.
	* The impulse will be consumed on the next tick.
	*/
	UFUNCTION(BlueprintCallable)
	void AddImpulse(const FVector Impulse);

	
	/** Sweeps for a valid floor beneath the character. If true, OutHitResult contains the sweep result */
	bool FindFloor(float SweepDistance, FHitResult& OutHitResult, const int NumTries);

	/** Checks if the hit result was for a valid floor based on component settings and floor slope */
	bool IsValidFloorHit(const FHitResult& FloorHit) const;

	/** Moves the character to the floor, but does not update velocity */
	void SnapToFloor(const FHitResult& FloorHit);
	
	
private:

	/** Perform one single movement step, with potential substeps */
	void StepMovement(float DeltaTime);

	/** Perform a single movement substep, returning the amount of time actually simulated in the substep */
	float SubstepMovement(const float DeltaTime);

	void UpdateVerticalMovement(const float DeltaTime);
	
	/** Explicit transition when landing on a floor while in the 'Falling' state */
	void Land();

	/** Explicit transition when a floor becomes invalid while in the 'Walking' state */
	void Fall();

	/**
	 * Function to be called when the player falls off the stage.
	 * By default, this sets the player's velocity so they will move back to the 'LastValidLocation'
	 */
	void Recover();

	void EndRecovery();
	
	void TickGravity(const float DeltaTime);
	
	void HandleInputVectors();


	// Update method wrapppers
	void UpdateRotation(const float DeltaTime);
	
	FRotator GetNewRotation(const float DeltaTime) const;
	
	FVector GetNewVelocity(const float DeltaTime);


	
	// Update utilities
	FVector HoldJump(const float DeltaTime);

	FVector ApplyFriction(const FVector& VelocityIn, const float DeltaTime) const;

	FVector ApplySlidingFriction(const FVector& VelocityIn, const float DeltaTime, const float Friction) const;


	// Basis/Floor Movement
	void MagnetToBasis(const float VelocityFactor, const float DeltaTime);

	void StoreBasisTransformPostUpdate();
	
	FVector GetRelativeBasisPosition() const;

	
	// Private impulse methods
	FVector ConsumeImpulse();

	void ClearImpulse();


	void AddAdjustment(const FVector& Adjustment);
	
	FVector ConsumeAdjustments();
	

	
	// Utilities
	static FVector ClampToPlaneMaxSize(const FVector& VectorIn, const FVector& Normal, const float MaxSize);

	bool SweepCapsule(const FVector Offset, FHitResult& OutHit, const bool bIgnoreInitialOverlap = false) const;
	bool SweepCapsule(const FVector InitialOffset, const FVector Offset, FHitResult& OutHit, const bool bIgnoreInitialOverlap = false) const;

	void RenderHitResult(const FHitResult& HitResult, const FColor Color = FColor::White, const bool bPersistent = false) const;

	bool CheckFloorValidWithinRange(const float Range, const FHitResult& HitResult) const;

	void HandleExternalOverlaps(const float DeltaTime);

	static bool MatchModes(const EPupMovementMode& Subject, std::initializer_list<EPupMovementMode> CheckModes);
	
public:	
	// Properties
	
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


	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient, Category = "Movement|Rotation")
	FRotator DesiredRotation = FRotator(0.f,0.f,0.f);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Rotation")
	float RotationSpeed = 360.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Rotation")
	bool bSlip = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Rotation", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float SlipFactor = 0.8f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement|Rotation")
	float CameraYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement|Rotation")
	float TurningDirection = 0.0f;

	

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement|Jumping")
	bool bCanJump = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Jumping")
	float JumpInitialVelocity = 300.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Jumping")
	float ApexVelocity = 500.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement|Jumping")
	float HoldJumpAcceleration = 400.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Jumping", meta = (ClampMin = 0.0f))
	float MaxJumpTime = 0.5f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Jumping", meta = (ClampMin = 0.0f))
	float CoyoteTime = 0.5f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Jumping", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float AirControlFactor = 0.2f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Jumping", meta = (ClampMin = 0.0f))
	float TerminalVelocity = 4000.f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement|Jumping")
	bool bGrounded = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement|Jumping")
	bool bJumping = false;


	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement|Anchored")
	FVector AnchorRelativeLocation;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement|Anchored")
	FVector AnchorWorldLocation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Anchored")
	float SnapVelocity = 1000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Anchored")
	float SnapRotationVelocity = 360.0f;


	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement|Planar")
	FVector FloorNormal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Planar")
	float MaxIncline = 60.0f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement|Planar")
	float MaxInclineZComponent = 0.5f;


	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Deflections")
	float DeflectionFriction = 800.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Deflections", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float DeflectionControlInfluence = 0.2f;

	/** If the player can regain control of the character early, by cancelling the velocity in the direction **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Deflections")
	bool bCanRegainControl = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Deflections")
	bool bCanJumpWhileDeflected = true;



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Recovery")
	float RecoveryTime = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Recovery")
	bool bIgnoreObstaclesWhenRecovering = true;
	
	/** The height offset added to our LastValidLocation when recovering. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Recovery")
	float RecoveryLevitationHeight = 100.0f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Movement|Recovery")
	FVector LastValidLocation;

	/** How far away from the edge we should be to consider a location 'safe' **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement|Recovery")
	float MinimumSafeRadius = 100.0f;

	
private:

	UPROPERTY(EditInstanceOnly, Category = "Movement")
	EPupMovementMode MovementMode = EPupMovementMode::M_Falling;	
	
	UPROPERTY(Transient)
	UPrimitiveComponent* CurrentFloorComponent;

	UPROPERTY(Transient)
	UPrimitiveComponent* AnchorTarget;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Movement|Planar|Basis");
	FVector LastBasisPosition;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Movement|Planar|Basis");
	FVector LocalBasisPosition;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Movement|Planar|Basis");
	FRotator LastBasisRotation;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Movement|Planar|Basis");
	FVector BasisRelativeVelocity;

	UPROPERTY(Transient)
	TArray<UPrimitiveComponent*> IgnoredComponentsForSweep;

	FVector DeflectDirection;
	
	float JumpAppliedVelocity;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Movement|Other")
	FVector PendingAdjustments = FVector::ZeroVector;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Movement|Other")
	FVector PendingImpulses;
	
	// Timer Handles
	FTimerHandle CoyoteTimerHandle;

	FTimerHandle JumpTimerHandle;

	FTimerHandle DeflectTimerHandle;
	
	FTimerHandle RecoveryTimerHandle;
};
