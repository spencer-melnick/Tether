// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "MovementComponent/PupMovementComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Tether/Core/Suspendable.h"
#include "Tether/Gameplay/Beam/BeamComponent.h"

#include "TetherCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UBeamComponent;
class UTopDownCameraComponent;


UCLASS(Blueprintable, HideCategories("Components"))
class ATetherCharacter : public APawn, public ISuspendable, public IBeamTarget
{
	GENERATED_BODY()

public:

	// Component name constants
	
	static const FName CapsuleComponentName;
	static const FName MovementComponentName;
	static const FName SkeletalMeshComponentName;
	static const FName GrabSphereComponentName;
	static const FName GrabHandleName;
	static const FName MantleHandleName;
	static const FName BeamComponentName;
	static const FName CameraComponentName;

	static const FName PickupTag;
	static const FName AnchorTag;


	explicit ATetherCharacter(const FObjectInitializer& ObjectInitializer);

	
	// Actor overrides
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void PossessedBy(AController* NewController) override;

	virtual void Suspend() override;
	virtual void Unsuspend() override;
	virtual void CacheInitialState() override;
	virtual void Reload() override;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPossessedDelegate, AController*)
	FOnPossessedDelegate& OnPossessedDelegate() { return PossessedDelegate; }
	
	// Beam target interface

	virtual UBeamComponent* GetBeamComponent_Implementation() const override { return BeamComponent; }

	
	// Movement functions
	void Jump();
	void StopJumping();
	
	void MoveX(const float Scale);
	void MoveY(const float Scale);

	void RotateX(const float Scale);
	void RotateY(const float Scale);
	
	void Interact();

	/**
	* Bounce the character off a obstacle.
	*
	* @param DeflectionNormal		The direction the character hit the obstacle in
	* @param DeflectionScale		How fast the character should be moving in the direction after deflection (disregarding elasticity)
	* @param InstigatorVelocity	Velocity of the object the caused the deflection
	* @param InstigatorFactor		How much the instigator's velocity affects deflection speed (e.g. fast moving objects deflecting more)
	* @param Elasticity				How much should the character's velocity impact the deflection speed
	* @param DeflectTime			How long (in seconds) until the character regains normal control
	* @param bLaunchVertically		Should the player be launched upwards or downwards at all?
	* @param bForceBreak			If true, force the character to let go of any anchor points. Defaults to false
	*/
	UFUNCTION(BlueprintCallable)
	void Deflect(
		FVector DeflectionNormal, float DeflectionScale,
		FVector InstigatorVelocity, float InstigatorFactor,
		float Elasticity, float DeflectTime, bool bLaunchVertically, bool bForceBreak = true);

	UFUNCTION(BlueprintCallable)
	void DeflectSimple(const FVector Velocity, float DeflectTime);


	// Accessors

	UFUNCTION(BlueprintCallable)
	FVector GetTetherTargetLocation() const;

	UFUNCTION(BlueprintCallable)
	FVector GetTetherEffectLocation() const;

	UFUNCTION(BlueprintCallable)
	bool GetIsJumping() const;

	UFUNCTION(BlueprintCallable)
	USkeletalMeshComponent* GetMeshComponent() const;

	UFUNCTION(BlueprintCallable)
	UCapsuleComponent* GetCapsuleComponent() const;

	UFUNCTION(BlueprintPure)
	bool IsAlive() const { return bAlive; }

	UFUNCTION(BlueprintPure)
	bool CanMove() const;

	UFUNCTION(BlueprintImplementableEvent)
	void OnJump();

	UFUNCTION(BlueprintCallable)
	void SetSnapFactor(const float Factor);

	void DragObject(AActor* Target, const FVector Normal);

	void DragObjectRelease();
	
	// Events

	virtual void HandlePenetration(const FHitResult& HitResult);
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EyeHeight = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tether")
	FVector TetherOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tether")
	FName TetherEffectSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCarryingObject = false;

	bool bAnchored = false;

	/** Maximum speed that this character can be launched at when hitting an obstacle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deflection")
	float MaxLaunchSpeed = 500.f;

	// Interaction settings

	/** Trace channel to use when checking for interactive objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interation")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel;

private:

	// Events

	UFUNCTION()
	void OnTetherExpired();
	

	// Animation tracking
	
	bool bAlive = true;

	// Interaction Handling

	void PickupObject(AActor* Object);
	void DropObject();

	UPROPERTY(Transient)
	AActor* CarriedActor;

	void AnchorToObject(AActor* Object) const;
	void AnchorToComponent(UPrimitiveComponent* Component, const FVector& Location = FVector::ZeroVector) const;
	void Release();

	bool bCompletedPickupAnimation = false;
	float SnapFactor = 0.0f;
	FVector InitialCarriedActorPosition;
	FRotator InitialCarriedActorRotation;


	
	// Components
public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	UCapsuleComponent* CapsuleComponent;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	UPupMovementComponent* MovementComponent;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Components")
	UTopDownCameraComponent* CameraComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components")
	USceneComponent* MantleLocationComponent;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Dragging")
	bool bDraggingObject;
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* GrabSphereComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* GrabHandle;

	UPROPERTY(VisibleAnywhere)
	UBeamComponent* BeamComponent;

	UPROPERTY(VisibleAnywhere, Transient, Category = "Dragging")
	AActor* DraggingActorObject;
	
	/**
	 * How far the center of the target object is from the center of the character
	 * Used to maintain the distance between them
	**/
    UPROPERTY(VisibleAnywhere, Transient)
    FVector DragOffset = FVector::ZeroVector;


	FOnPossessedDelegate PossessedDelegate;

	FPupMovementComponentState InitialState;
};