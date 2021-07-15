// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PupMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class TETHER_API UPupMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UPupMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

	bool SweepCapsule(const FVector Offset, FHitResult& OutHit, bool bIgnoreFloor = false) const;

	void AddImpulse(const FVector Impulse);
	
	// Accessor Overrides
	
	virtual float GetMaxSpeed() const override;


	// Movement Logic

	bool CheckForFloor(const float Distance, FVector& Location);

	
private:
	
	void UpdateGravity(float DeltaTime);
	
	void HandleInputAxis();
	
	FRotator GetNewRotation(float DeltaTime);
	
	FVector GetNewVelocity(float DeltaTime);

	void ApplyFriction(float DeltaTime);

	
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
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Movement")
	FVector DirectionVector;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsWalking = false;

	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement: Rotation")
	FRotator DesiredRotation = FRotator(0.f,0.f,0.f);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement: Rotation")
	float RotationSpeed = 360.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement: Rotation")
	bool bSlip = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement: Rotation")
	float SlipFactor = 0.8;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement: Rotation")
	float CameraYaw = 0.f;


	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement: Jumping")
	float AirControlFactor = 0.2;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement: Jumping")
	float TerminalVelocity = -4000.f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Movement: Jumping")
	bool bGrounded = false;

};
