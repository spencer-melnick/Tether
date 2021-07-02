// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"

#include "TetherCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS(Blueprintable)
class ATetherCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	// Component name constants
	
	static const FName CameraComponentName;
	static const FName SpringArmComponentName;
	static const FName GrabSphereComponentName;
	static const FName GrabHandleName;

	static const FName PickupTag;

	
	ATetherCharacter();

	
	// Actor overrides
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;

	
	// Character overrides
	
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void Falling() override;
	virtual void OnJumped_Implementation() override;

	
	// Movement functions

	void MoveX(float Scale);
	void MoveY(float Scale);
	void RotateX(float Scale);
	void RotateY(float Scale);
	void GrabObject();

	UFUNCTION(BlueprintCallable)
	void SetGroundFriction(float GroundFriction);


	// Accessors

	// UFUNCTION(BlueprintCallable)
	// UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	UFUNCTION(BlueprintPure)
	USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; }

	UFUNCTION(BlueprintCallable)
	FVector GetTetherTargetLocation() const;

	UFUNCTION(BlueprintCallable)
	FVector GetTetherEffectLocation() const;

	UFUNCTION(BlueprintPure)
	bool IsAlive() const { return bAlive; }

	UFUNCTION(BlueprintPure)
	bool CanMove() const;

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

	/** Ground friction when this character isn't being bounced around */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deflection")
	float NormalFriction = 8.f;

	/** Ground friction when this character is bounced by an obstacle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deflection")
	float BounceFriction = 4.f;

	/** Maximum speed that this character can be launched at when hitting an obstacle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deflection")
	float MaxLaunchSpeed = 500.f;

private:

	// Events

	UFUNCTION()
	void OnTetherExpired();
	

	// Components
	
	// UPROPERTY(VisibleAnywhere, Category="Components")
	// UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, Category="Components")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, Category="Components")
	USphereComponent* GrabSphereComponent;

	UPROPERTY(VisibleAnywhere, Category="Components")
	USceneComponent* GrabHandle;

	UPROPERTY(Transient)
	AActor* GrabbedObject;


	// Jump tracking

	FTimerHandle CoyoteJumpTimerHandle;
	bool bCoyoteJumpAvailable = false;

	
	// Obstacle Deflection
	
	FTimerHandle DeflectTimerHandle;

	bool bIsBouncing = false;
	

	// Animation tracking

	bool bAlive = true;
	
};
