// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "PupMovementComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Tether/Gameplay/BeamComponent.h"

#include "TetherCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UBeamComponent;


UCLASS(Blueprintable)
class ATetherCharacter : public APawn, public IBeamTarget
{
	GENERATED_BODY()

public:

	// Component name constants
	
	static const FName CapsuleComponentName;
	static const FName MovementComponentName;
	static const FName SkeletalMeshComponentName;
	static const FName GrabSphereComponentName;
	static const FName GrabHandleName;
	static const FName BeamComponentName;

	static const FName PickupTag;
	static const FName AnchorTag;


	ATetherCharacter(const FObjectInitializer& ObjectInitializer);

	
	// Actor overrides
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	void Jump();
	void StopJumping();


	// Character overrides
	
	void Falling();
	void OnJumped_Implementation();


	// Beam target interface

	virtual UBeamComponent* GetBeamComponent_Implementation() const override { return BeamComponent; }

	
	// Movement functions

	void MoveX(const float Scale);
	void MoveY(const float Scale);
	void Interact();

	UFUNCTION(BlueprintCallable)
	void SetGroundFriction(float GroundFriction);

	/**
	* Bounce the character off a obstacle.
	*
	* @param DeflectionNormal		The direction the character hit the obstacle in
	* @param DeflectionScale		How fast the character should be moving in the direction after deflection (disregarding elasticity)
	* @param InstigatorVelocity	Velocity of the object the caused the deflection
	* @param InstigatorFactor		How much the instigator's velocity affects deflection speed (e.g. fast moving objects deflecting more)
	* @param Elasticity			How much should the character's velocity impact the deflection speed
	* @param DeflectTime			How long (in seconds) until the character regains normal control
	* @param bLaunchVertically		Should the player be launched upwards or downwards at all?
	*/
	UFUNCTION(BlueprintCallable)
	void Deflect(
		FVector DeflectionNormal, float DeflectionScale,
		FVector InstigatorVelocity, float InstigatorFactor,
		float Elasticity, float DeflectTime, bool bLaunchVertically);


	// Accessors

	UFUNCTION(BlueprintCallable)
	FVector GetTetherTargetLocation() const;

	UFUNCTION(BlueprintCallable)
	FVector GetTetherEffectLocation() const;

	UFUNCTION(BlueprintCallable)
	USkeletalMeshComponent* GetMeshComponent() const;

	UFUNCTION(BlueprintCallable)
	UCapsuleComponent* GetCapsuleComponent() const;

	UFUNCTION(BlueprintPure)
	bool IsAlive() const { return bAlive; }

	UFUNCTION(BlueprintPure)
	bool CanMove() const;

	
	// Events

	virtual void HandlePenetration(const FHitResult& SweepResult);


	// Editor properties
	
	// Jump controls

	/** How long after falling a character can jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Jump")
	float CoyoteTime = 0.1f;

	// Tether settings

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tether")
	FVector TetherOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tether")
	FName TetherEffectSocket;

	bool bCarryingObject = false;

	bool bAnchored = false;

	/** Ground friction when this character isn't being bounced around */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deflection")
	float NormalFriction = 8.f;

	/** Ground friction when this character is bounced by an obstacle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deflection")
	float BounceFriction = 4.f;

	/** Maximum speed that this character can be launched at when hitting an obstacle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deflection")
	float MaxLaunchSpeed = 500.f;

	// Interaction settings

	/** Trace channel to use when checking for interactive objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interation")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel;

private:

	// Events

	UFUNCTION()
	void OnTetherExpired();

	
	// Jump tracking

	FTimerHandle CoyoteJumpTimerHandle;
	bool bCoyoteJumpAvailable = false;

	
	// Obstacle Deflection
	
	FTimerHandle DeflectTimerHandle;

	bool bIsBouncing = false;
	

	// Animation tracking
	
	bool bAlive = true;

	// Interaction Handling
	
	void PickupObject(AActor* Object);
	void DropObject();

	UPROPERTY(Transient)
	AActor* CarriedActor;

	void AnchorToObject(AActor* Object);
	void ReleaseAnchor();


	
	// Components
public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Components")
	UCapsuleComponent* CapsuleComponent;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Components")
	UPupMovementComponent* MovementComponent;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Components")
	USkeletalMeshComponent* SkeletalMeshComponent;

private:
	UPROPERTY(VisibleAnywhere, Category="Components")
	USphereComponent* GrabSphereComponent;

	UPROPERTY(VisibleAnywhere, Category="Components")
	USceneComponent* GrabHandle;

	UPROPERTY(VisibleAnywhere)
	UBeamComponent* BeamComponent;
};
