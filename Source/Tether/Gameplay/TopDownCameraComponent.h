// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"

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
	
private:

	FVector CalcDeltaLocation(const float DeltaTime);

	FRotator CalcDeltaRotation(const float DeltaTime);
	
	FVector2D ConvertVectorToViewSpace(const FVector& WorldLocation) const;
	
	FVector2D CalcSubjectScreenLocations();
	
	TArray<AActor*> GetPlayerControlledActors() const;

	FVector AverageLocationOfTargets(TArray<AActor*> Targets);

	float GetMinimumDistance();
	
	FVector GetWorldOffset() const;

	FRotator ConsumeRotations();

	// MEMBER VARIABLES
	
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking")
	bool bKeepAllTargetsInFrame;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Tracking")
	FVector DesiredFocalPoint;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Tracking")
	FVector FocalPoint;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking")
	float TrackingSpeed = 0.2f;



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking | Rotation")
	float RotationSensitivity = 5.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Tracking | Rotation")
	FRotator RotationalVelocity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking | Rotation")
	bool bLockRotation = false;
	
	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking | Subjects")
	TArray<AActor*> Subjects;

	/// Subject locations in screen space, ranging  [-1.0 : 1.0]
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Tracking | Subjects")
	TArray<FVector2D> SubjectLocations;

	/// Whether or not we try to get ahead of the subjects, based on their velocity.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking | Subjects")
	bool bTrackAheadOfMotion = true;

	/// How far ahead of the subject we track, in seconds
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking | Subjects")
	float TrackAnticipationTime = 1.0f;

	

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient, Category = "Tracking | Zoom")
	float SubjectDistance;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Tracking | Zoom")
	float DesiredSubjectDistance;

	/// The closest we can get to the subjects. This value will prevent the camera from zooming in too close.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking | Zoom")
	float MinimumSubjectDistance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking | Zoom")
	float ZoomSpeed;
	
private:

	FVector2D ScreenSize;

	FRotator PendingRotations;
};