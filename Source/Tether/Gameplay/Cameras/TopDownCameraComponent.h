// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "Tether/Character/TetherCharacter.h"

#include "TopDownCameraComponent.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent))
class TETHER_API UTopDownCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
	
public:	
	UTopDownCameraComponent();

protected:
	
	virtual void BeginPlay() override;

public:
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void AddCameraRotation(const FRotator Rotator);

	void RecordScreenSize(AController* Controller);

	void ResetLocation();

private:

	FVector CalcDeltaLocation(const float DeltaTime);

	float CalcDeltaZoom(const float DeltaTime);
	
	FRotator CalcDeltaRotation(const float DeltaTime);
	
	FVector2D ConvertVectorToViewSpace(const FVector& WorldLocation) const;

	float GetMinimumDistance();
	
	FVector GetWorldOffset() const;

	FRotator ConsumeRotations();
	
	// MEMBER VARIABLES
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "User", meta = (ClampMin = 0.0f, ClampMax = 10.0f))
	float YSensitivity = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "User")
	bool bInvertCameraY = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "User", meta = (ClampMin = 0.0f, ClampMax = 10.0f))
	float XSensitivity = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "User")
	bool bInvertCameraX = false;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking")
	bool bTrackParent = true;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Tracking")
	FVector DesiredFocalPoint;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Tracking")
	FVector FocalPoint;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking")
	float TrackingSpeed = 0.2f;

	/// The maximum distance the camera can be anticipating the character's movement.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking")
	float MaxAnticipation = 200.0f;

	/** The Rotation speed of the camera in deg/s, scaled by the input factor **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Rotation")
	float RotationSpeed = 5.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Rotation", meta = (ClampMin = -90.0f, ClampMax = 90.0f))
	float MinPitch = -85.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Rotation", meta = (ClampMin = -90.0f, ClampMax = 90.0f))
	float MaxPitch = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Tracking|Rotation")
	FRotator RotationalVelocity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Rotation")
	bool bLockRotation = false;

	/// How much the camera rotates towards the direction the primary actor is going.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Rotation", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float CopyRotationFactor = 0.0f;
	
	
	/// The actors we are tracking. If we track more than one actor, the first actor in the list will be our 'primary' actor.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Subjects")
	ATetherCharacter* Subject;
		
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Tracking|Subjects")
	FVector SubjectLocation;

	/// Whether or not we try to get ahead of the subjects, based on their velocity.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Subjects")
	bool bTrackAheadOfMotion = true;

	/// How far ahead of the subject we track, in seconds
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Subjects")
	float TrackAnticipationTime = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Zoom")
	bool bUseSpringArm = true;
	/** If true the camera zoom will not be changed once the minimum and maximum distances are applied */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Zoom")
	bool bLockZoom = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient, Category = "Tracking|Zoom")
	float SubjectDistance;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Tracking|Zoom")
	float DesiredSubjectDistance;

	/// The closest we can get to the subjects. This value will prevent the camera from zooming in too close.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Zoom")
	float MinimumSubjectDistance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Zoom")
	float MaximumSubjectDistance;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking|Zoom")
	float ZoomSpeed;
	
private:

	FVector2D ScreenSize;

	FRotator PendingRotations;

	float InitialFOV;
};