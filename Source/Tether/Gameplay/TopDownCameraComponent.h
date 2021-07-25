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
	
private:

	FVector ConvertVectorToViewSpace(const FVector& WorldLocation) const;
	
	FVector2D CalcMinScreenBounds() const;
	
	TArray<AActor*> GetPlayerControlledActors() const;

	FVector AverageLocationOfTargets(TArray<AActor*> Targets);

	float GetMinimumDistance() const;
	
	FVector GetWorldOffset() const;	

	// MEMBER VARIABLES
	
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking")
	bool bKeepAllTargetsInFrame;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking")
	TArray<AActor*> Subjects;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Tracking")
	FVector DesiredFocalPoint;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Tracking")
	FVector FocalPoint;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking | Zoom")
	float SubjectDistance;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Tracking | Zoom")
	float DesiredSubjectDistance;

	/**
	 * @details {The closest we can get to the subjects. This value will prevent the camera from zooming in too close.}
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking | Zoom")
	float MinimumSubjectDistance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tracking | Zoom")
	float ZoomSpeed;

private:

};