// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PupMovementComponent.generated.h"

/**
 * 
 */

struct FPupMovementComponentState;
UENUM(BlueprintType)
enum class EPupMovementMode : uint8
{
	M_None		UMETA(DisplayName = "None"),
	M_Walking	UMETA(DisplayName = "Walking"),
	M_Falling	UMETA(DisplayName = "Falling"),
	M_Anchored	UMETA(DisplayName = "Anchored"),
	M_Deflected UMETA(DisplayName = "Deflected"),
	M_Recover	UMETA(DisplayName = "Recover"),
	M_Dragging	UMETA(DisplayName = "Dragging")
};


UCLASS(HideCategories = ("NavMovement", "MovementComponent", "PlanarMovement", "ComponentTick"))
class TETHER_API UPupMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UPupMovementComponent();
	UPupMovementComponent(const FObjectInitializer& ObjectInitializer);
	
	// Overrides
	virtual  void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual float GetMaxSpeed() const override;

	virtual float GetGravityZ() const override;

	virtual bool ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation) override;
	
	
	// Movement mode transitions
	/**
	 * Set the player's new movement mode. Automatically handles any transitions.
	 * @returns whether or not the movement mode could be changed.
	 **/
	UFUNCTION(BlueprintCallable)
	bool SetMovementMode(const EPupMovementMode& NewMovementMode);

	/** Return the player's movement mode. **/
	UFUNCTION(BlueprintCallable)
	EPupMovementMode GetMovementMode() const;
	
	/**
	* Set the player's movement mode to whatever mode is appropriate, given their position and velocity.
	* For instance, a player in the air will be set to 'Falling'.
	* This should be called when an object does not have access to the
	* necessary context to determine the correct movement mode.
	*/
	UFUNCTION(BlueprintCallable)
	void SetDefaultMovementMode();

	UFUNCTION(BlueprintCallable)
	void SetWorldLocation(const FVector Location);

	/** Grab onto a specific location, preventing the player from falling or moving */
	UFUNCTION(BlueprintCallable)
	void AnchorToComponent(UPrimitiveComponent* AnchorTargetComponent, const FVector& Location);

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

	/** Try to grab onto a ledge **/
	void Mantle();

	void Dash();
	
	/** Finish climbing up from a ledge **/
	UFUNCTION(BlueprintCallable)
	void EndMantleClimb();

	void BeginDraggingObject(const FVector& TargetNormal);

	void EndDraggingObject();
	
	/**
	* Add velocity directly to the player.
	* The impulse will be consumed on the next tick.
	*/
	UFUNCTION(BlueprintCallable)
	void AddImpulse(const FVector Impulse);

	// Called when being pushed by an object
	void Push(const FHitResult& HitResult, const FVector ImpactVelocity, UPrimitiveComponent* Source);
	
	void AddRootMotionTransform(const FTransform& RootMotionTransform);
	
	UFUNCTION(BlueprintCallable)
	void IgnoreActor(AActor* Actor);

	UFUNCTION(BlueprintCallable)
	void UnignoreActor(AActor* Actor);

	UFUNCTION(BlueprintCallable)
	void SupressInput();

	UFUNCTION(BlueprintCallable)
	void UnsupressInput();

	void HitWall(const FHitResult& HitResult);
	
	/** Sweeps for a valid floor beneath the character. If true, OutHitResult contains the sweep result */
	bool FindFloor(float SweepDistance, FHitResult& OutHitResult, const int NumTries);

	/** Checks if the hit result was for a valid floor based on component settings and floor slope */
	bool IsValidFloorHit(const FHitResult& FloorHit) const;

	/** Moves the character to the floor, but does not update velocity */
	void SnapToFloor(const FHitResult& FloorHit);

	void ResetState(FPupMovementComponentState* State);

	void PauseTimers();

	void UnPauseTimers();
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMovementModeChanged, EPupMovementMode, OldMovementMode, EPupMovementMode, NewMovementMode);
	FMovementModeChanged& OnMovementModeChanged(EPupMovementMode, EPupMovementMode) { return MovementModeChanged; }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FJumpEvent, const FVector, FloorLocation, const bool, bInitialJump);
	FJumpEvent& OnJumpEvent(const FVector, const bool) { return JumpEvent; }
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLandEvent, const FVector, FloorLocation, const float, ImpactVelocity, UPrimitiveComponent*, FloorComponent);
	FLandEvent& OnLandEvent(const FVector, const float) { return LandEvent; }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMantleEvent);
	FMantleEvent& OnMantleEvent() { return MantleEvent; }

	DECLARE_MULTICAST_DELEGATE(FForceDragReleaseEvent);
	FForceDragReleaseEvent& OnForceDragReleaseEvent() { return ForceDragReleaseEvent; }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDashEvent, const FVector, DashDirection);
	FDashEvent& OnDashEvent() { return DashEvent; }
	
private:

	/** Perform one single movement step, with potential substeps */
	void StepMovement(float DeltaTime);

	/** Perform a single movement substep, returning the amount of time actually simulated in the substep */
	float SubstepMovement(const float DeltaTime);

	void UpdateVerticalMovement(const float DeltaTime);
	
	/** Explicit transition when landing on a floor while in the 'Falling' state */
	void Land(const FVector& FloorLocation, const float ImpactVelocity, UPrimitiveComponent* FloorComponent);

	void WallJump();
	
	/** Explicit transition when a floor becomes invalid while in the 'Walking' state */
	void Fall();

	/**
	 * Function to be called when the player falls off the stage.
	 * By default, this sets the player's velocity so they will move back to the 'LastValidLocation'
	 */
	void Recover();

	void EndDash();
	
	/**
	 * Timer function to be called when the player has finished their recovery.
	 * By default, this teleports the player to their last safe location, in case the player
	 * was de-synced during this time.
	 **/
	void EndRecovery();

	/** Climb up over the ledge and return to default movement mode **/
	void ClimbMantle();

	/** Slide along the ledge we are currently holding to the right **/
	bool EdgeSlide(const float Scale, const float DeltaTime);

	/** Transform any accumulated input vectors to be relative to the player's viewpoint, and store them as a property. **/
	void HandleInputVectors();


	/** Return the new rotation for this tick. **/
	FRotator GetNewRotation(const float DeltaTime);
	/** Return the new velocity for this tick. **/
	FVector GetNewVelocity(const float DeltaTime);


	
	// Update utilities
	/** Add additional velocity to the player if they hold down 'Jump' **/
	FVector HoldJump(const float DeltaTime);

	/** Get the new velocity after stopping the player's movement along whatever plane they are attached to. **/
	FVector ApplyFriction(const FVector& VelocityIn, const float DeltaTime);

	/** Get the new velocity after Friction has been applied. **/
	FVector ApplySlidingFriction(const FVector& VelocityIn, const float DeltaTime, const float Friction) const;


	// Basis/Floor Movement
	void MagnetToBasis(const float VelocityFactor, const float DeltaTime);

	void HandlePushes(const float DeltaTime);
	
	/** Update our relative basis transform to reflect any movements the player has made themself. **/
	void StoreBasisTransformPostUpdate();

	/** Calculate the player's position in world space, in case the floor they are standing on has moved. **/
	FVector GetRelativeBasisPosition() const;

	
	// Private impulse methods
	/** Get the total value of pending impulses, and empty their value. **/
	FVector ConsumeImpulse();
	/** Remove all pending impulses, and don't get their value. **/
	void ClearImpulse();

	/** Register a change in velocity caused by a penetration or other overlap. **/
	void AddAdjustment(const FVector& Adjustment);

	/** Get the total velocity of adjustments we had to make this step, and empty the value. **/
	FVector ConsumeAdjustments();
	

	
	// Utilities
	/** Scale an FVector to a max size along a plane, similar to ClampToMaxSize2D. **/
	static FVector ClampToPlaneMaxSize(const FVector& VectorIn, const FVector& Normal, const float MaxSize);

	bool SweepCapsule(const FVector Offset, FHitResult& OutHit, const bool bIgnoreInitialOverlap = false) const;
	bool SweepCapsule(const FVector InitialOffset, const FVector Offset, FHitResult& OutHit, const bool bIgnoreInitialOverlap = false) const;

	
	/** Utility function for rendering HitResults. Will only fire if bDrawMovementDebug is checked. **/
	void RenderHitResult(const FHitResult& HitResult, const FColor Color = FColor::White, const bool bPersistent = false) const;

	void HandleRootMotion();
	
	/** Static utility for finding if the player is at least 'Range' units from the edge of a surface. **/
	static bool CheckFloorValidWithinRange(const float Range, const FHitResult& HitResult);

	/**
	 * Check for, and handle, any overlaps that may have occurred without firing a hit event.
	 * This typically occurs when the player is stationary and an object has moved 'into' them.
	 * By default, this will also apply an impulse proportional to how quickly the player moved
	 * out of the way.
	 **/
	void HandleExternalOverlaps(const float DeltaTime);

	/** Static utility function for checking if a PupMovementMode matches any of a list of modes. **/
	static bool MatchModes(const EPupMovementMode& Subject, std::initializer_list<EPupMovementMode> CheckModes);
	
	FVector GetLocationRelativeToComponent(const FVector& WorldLocation, USceneComponent* Component) const;

public:	
	// Properties



	
	/* ========================== SPEED PROPERTIES ========================== */
	/**
	 * The maximum running velocity the player can achieve by running.
	 * External forces can accelerate the player further, but they will be unable
	 * to maintain that speed.
	 * To control the absolute maximum velocity, see: 'TerminalVelocity'
	 **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed", meta = (ClampMin = 0.0f))
	float MaxSpeed;

	UPROPERTY(VisibleInstanceOnly, Category = "Speed", AdvancedDisplay)
	float Speed;

	/** How quickly the player gains speed (in units/s2) when the control stick is pushed all the way. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed", meta = (ClampMin = 0.0f))
	float MaxAcceleration = 200.f;

	/** How quickly the player stops (in units/s2) when the control stick is released. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed|Friction", meta = (ClampMin = 0.0f))
	float BreakingFriction = 2000.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed|Dash")
	float DashSpeed = 400.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed|Dash")
	float DashTime = 0.5f;
	
	UPROPERTY(BlueprintReadWrite, VisibleInstanceOnly, Category = "Speed|Dash")
	FVector DashDirection;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Speed|Dash")
	bool bCanDash = true;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Speed|Dash")
	bool bDashing = false;


	
	/* ========================== INPUT PROPERTIES ========================== */
	/**
	 * Where the player is moving, relative to the camera's yaw.
	 * Converted from accumulated input vectors, with a maximum length of 1.
	 **/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Input", meta = (MakeEditWidget = true))
	FVector DirectionVector;

	/**
	 * How far the player is pushing the control stick in any given direction
	 * On Keyboard, defaults to 1.
	 **/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Input")
	float InputFactor;

	/**
	 * Is the player receiving directional input from the controller
	 * that is VALID, i.e., causing any movement -- linear or rotational.
	 * Used to drive animations.
	 **/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Input")
	bool bIsWalking = false;




	
	
	/* ========================== ROTATION PROPERTIES ========================== */
	/**
	 * How close are we to the maximum (running) velocity, on a linear scale.
	 * 0 indicates we are not moving at all, while 1
	 * indicates we are running at maximum velocity.
	 **/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, AdvancedDisplay, Category = "Rotation")
	float MovementSpeedAlpha = 0.0f;

	/** The absolute maximum velocity the player can go in any movement state. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping", meta = (ClampMin = 0.0f))
	float TerminalVelocity = 4000.f;

	/** Where the player is turning towards. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient, Category = "Rotation", meta = (MakeEditWidget))
	FRotator DesiredRotation = FRotator(0.f,0.f,0.f);

	/** How fast (in degrees/s) the player turns to face the direction of the input. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rotation", meta = (ClampMin = 0.0f))
	float RotationSpeed = 360.f;

	/** Will the player rotate slower as they move faster? Simulates decreased static friction when running. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rotation|Slip")
	bool bSlip = true;

	/**
	 * How much the player's rotation speed should be multiplied by when moving fast.
	 * Scales linearly with velocity - if this value is 0 and the player is moving at
	 * their maximum speed, they will be unable to turn.
	 * If at 1, the player will turn at the same speed regardless of how fast they
	 * are moving across the floor.
	 **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rotation|Slip", meta = (ClampMin = 0.0f, ClampMax = 1.0f, EditCondition = "bSlip"))
	float SlipFactor = 0.8f;

	/**
	 * The yaw of the camera component, so that input is relative to the camera's rotation (yaw only).
	 * Automatically found from the current camera.
	 **/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Rotation")
	float CameraYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Rotation")
	float TurningDirection = 0.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Rotation")
	float AverageTurningSpeed = 0.0f;


	
	

	/* ========================== JUMPING PROPERTIES ========================== */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping|Floor", meta = (ClampMin = 0.0f))
	float FloorSnapDistance = 5.0f;

	/**
	* Is the player on a valid floor? See: Movement | Planar for
	* controlling what is considered a valid floor.
	**/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Jumping")
	bool bGrounded = false;


	/** Can the player jump? Determined by internal implementation. **/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Jumping")
	bool bCanJump = false;

	/** The initial velocity (in units/s) applied as an impulse, when jumping **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping", meta = (ClampMin = 0.0f))
	float JumpInitialVelocity = 300.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping", meta = (ClampMin = 0.0f))
	float ApexVelocity = 500.0f;

	/**
	 * How much the player accelerates (in units/s2) when the jump is held, after the initial velocity is applied.
	 * Derived from the ApexVelocity and HoldJumpAcceleration values.
	 **/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Jumping")
	float HoldJumpAcceleration = 400.0f;

	/** The maximum time the Jump can be held for, in seconds. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping", meta = (ClampMin = 0.0f))
	float MaxJumpTime = 0.5f;

	/**
	 * How long after falling (in seconds) can the player still jump? Setting this value to 0
	 * will prevent the player from jumping at all immediately after leaving the floor.
	 * Recommended to set this value to compensate for player reaction time and other
	 * sources of latency.
	 **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping", meta = (ClampMin = 0.0f))
	float CoyoteTime = 0.5f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float AirControlFactor = 0.2f;

	/**
	 * Does the character move forward along the direction is facing?
	 * If true, the character accelerates only forwards and rotates to accomodate the direction of input.
	 * If false, the acceleration will be applied regardless of the player's rotation, and they will
	 * begin to rotate in that direction.
	 **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping")
	bool bTurnFirst = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping|Double Jump")
	bool bDoubleJump = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping|Double Jump", meta = (ClampMin = 0.0f, ClampMax = 1.0f, EditCondition = "bDoubleJump"))
	float DoubleJumpAccelerationFactor = 0.5f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Jumping|Double Jump")
	bool bJumping = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jumping|Wall Jump")
	float WallJumpDisableTime = 0.2f;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Jumping|Wall Jump")
	bool bWallJumpDisabledControl = false;



	
	/* ========================== ANCHORING PROPERTIES ========================== */
	/** Is the player grabbing a ledge, instead of holding an object? **/
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Anchored|Mantling")
	bool bMantling = false;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Transient, Category = "Anchored|Mantling")
	bool bCanMantle = true;

	/** How close a wall must be to the player before they can grab on to it. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Anchored|Mantling", meta = (ClampMin = 0.0f))
	float GrabRangeForward = 20.0f;

	/** How far above the eye location the player can grab onto a ledge. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Anchored|Mantling", meta = (ClampMin = 0.0f))
	float GrabRangeTop = 10.0f;

	/** How far below the eye location the player can grab onto a ledge. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Anchored|Mantling", meta = (ClampMin = 0.0f))
	float GrabRangeBottom = 10.0f;

	/**
	 * How fast the player must be falling before they can grab onto the ledge.
	 * The apex of the height is 0.0 (unit/s).
	 * A negative value will allow the player to grab onto a ledge, even
	 * if they are still moving upward.
	 **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Anchored|Mantling")
	float MaximumGrabVelocity = -100.0f;

	/**
	 * How far the ledge can deviate from a perfectly 'level' floor.
	 * At 1.0f, the ledge must be completely level, and at 0.0f
	 * the ledge could be vertical.
	 * This value is recommended to be somewhere between 0.5f and 0.95f
	 * to account for errors in floating point arithmetic.
	 **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Anchored|Mantling", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float LedgeDeviation = 0.9f;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Transient, Category = "Anchored|Mantling")
	FVector LedgeDirection;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Anchored")
	FVector DesiredAnchorLocation;
	
	/** How quickly the player should move to their anchor, in units/s **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Anchored", meta = (ClampMin = 0.0f))
	float SnapVelocity = 1000.0f;

	/** How quickly the player should rotate to face their anchor, in degrees/s **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Anchored", meta = (ClampMin = 0.0f))
	float SnapRotationVelocity = 360.0f;	

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Anchored|Wall Slide")
	bool bWallSliding = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient, Category = "Anchored|Wall Slide")
	FVector WallNormal;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Transient, Category = "Anchored|Wall Slide|Scramble")
	bool bCanScramble = false;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Transient, Category = "Anchored|Wall Slide|Scramble")
	bool bWallScrambling = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Anchored|Wall Slide|Scramble")
	float WallScrambleTime = 10.0f;
	

	
	/**
	 * The normal of whatever floor surface the player is standing on.
	 * Used for calculating movement along the plane.
	 **/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Planar", meta = (MakeEditWidget))
	FVector FloorNormal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Planar", meta = (ClampMin = 0.0f, ClampMax = 90.0f))
	float MaxIncline = 60.0f;

	/**
	 * The smallest value the Z component of our FloorNormal can be,
	 * before it will no longer be considered a valid floor and the
	 * player will instead begin to slide down it.
	 * Calculated whenever MaxIncline is changed; Cached value.
	 **/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Planar")
	float MaxInclineZComponent = 0.5f;


	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Planar", Meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float MaxWallDeviation = 0.2f;

	
	/** The deceleration of the player when being set to the Deflected state by another object **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Deflections", meta = (ClampMin = 0.0f))
	float DeflectionFriction = 800.0f;

	/** Factor representing how much the player can still be controlled when Deflected **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Deflections", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float DeflectionControlInfluence = 0.2f;

	/** If the player can regain control of the character early, by cancelling the velocity in the direction **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Deflections")
	bool bCanRegainControl = true;

	/** Can the player jump, even if they are being Deflected? Player must still be on a valid floor. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Deflections")
	bool bCanJumpWhileDeflected = true;




	
	/** How long it takes the player to return to the stage after falling. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Recovery", meta = (ClampMin = 0.0f))
	float RecoveryTime = 1.0f;

	/** Should the player be able to pass through any/all objects when recovering? **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Recovery")
	bool bIgnoreObstaclesWhenRecovering = true;
	
	/** The height offset added to our LastValidLocation when recovering. **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Recovery")
	float RecoveryLevitationHeight = 100.0f;

	/** The last location on a floor that was sufficiently far from the edge. **/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Recovery")
	FVector LastValidLocation;

	/** How far away from the edge we should be to consider a location 'safe' **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Recovery", meta = (ClampMin = 0.0f))
	float MinimumSafeRadius = 100.0f;




	
	/* ========================== DRAGGING PROPERTIES ========================== */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dragging")
	bool bDragOnlyWhenGrounded = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dragging")
	float DragSpeed = 100.0f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Dragging")
	bool bIsDraggingSomething = false;
	
	// UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Dragging")
	// UPrimitiveComponent* ObjectDragging;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Dragging")
	FVector DraggingFaceNormal;
	
	
private:

	UPROPERTY(VisibleAnywhere, Category = "Speed")
	EPupMovementMode MovementMode = EPupMovementMode::M_Falling;	

	bool bSupressingInput = false;
	
	/** Pointer to the component that is serving as the floor. **/
	UPROPERTY(Transient)
	UPrimitiveComponent* BasisComponent;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Basis")
	bool bAttachedToBasis = false;

	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Basis|Relative")
	FVector LocalBasisPosition;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Basis|Relative");
	FVector BasisRelativeVelocity;

		
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Basis|History")
	FRotator BasisRotationLastTick;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Basis|History")
	FVector BasisPositionLastTick;

	/**
	 * Components to ignore when looking for our floor surface,
	 * that have either overlapped our capsule, or become invalid
	 * in some other way.
	 * Should be cleared after the floor has been successfully found.
	 **/
	UPROPERTY(Transient)
	TArray<UPrimitiveComponent*> InvalidFloorComponents;

	/**
	 * Which actors should we ignore for ALL movement sweeps?
	 * E.g., actors being held by the player.
	 * This array must be cleared manually.
	 **/
	UPROPERTY(Transient, VisibleInstanceOnly)
	TArray<AActor*> IgnoredActors;

	/** Which direction the player was launched by a deflection. Used to calculate when to regain control. **/
	FVector DeflectDirection;

	/** How much of the ApexJumpVelocity has been applied so far to the player? **/
	float JumpAppliedVelocity;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Jumping")
	bool bCanDoubleJump = false;

	
	FVector PendingAdjustments = FVector::ZeroVector;
	FVector PendingImpulses = FVector::ZeroVector;
	FVector PendingPushes = FVector::ZeroVector;
	FTransform PendingRootMotionTransforms = FTransform::Identity;
	
	// Timer Handles
	FTimerHandle CoyoteTimerHandle;
	FTimerHandle JumpTimerHandle;
	FTimerHandle DeflectTimerHandle;
	FTimerHandle RecoveryTimerHandle;
	FTimerHandle MantleTimerHandle;
	FTimerHandle MantleDebounceTimerHandle;
	FTimerHandle DashTimerHandle;

	FTimerHandle EdgeScrambleTimerHandle;

	UPROPERTY(BlueprintAssignable)
	FMovementModeChanged MovementModeChanged;
	
	UPROPERTY(BlueprintAssignable)
	FLandEvent LandEvent;

	UPROPERTY(BlueprintAssignable)
	FJumpEvent JumpEvent;

	UPROPERTY(BlueprintAssignable)
	FMantleEvent MantleEvent;

	FForceDragReleaseEvent ForceDragReleaseEvent;

	UPROPERTY(BlueprintAssignable)
	FDashEvent DashEvent;
};

USTRUCT()
struct FPupMovementComponentState
{
	GENERATED_BODY()

	FPupMovementComponentState();
	FPupMovementComponentState(const UPupMovementComponent* Src);

	EPupMovementMode GetMovementMode() const { return MovementMode; }
	FTransform GetTransform() const { return Transform; }
	FVector GetVelocity() const { return Velocity; }

private:
	EPupMovementMode MovementMode;
	FTransform Transform;
	FVector Velocity;
};
