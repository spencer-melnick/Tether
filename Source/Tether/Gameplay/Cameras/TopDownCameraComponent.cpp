// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "TopDownCameraComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Tether/Character/TetherCharacter.h"

// Sets default values
UTopDownCameraComponent::UTopDownCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

// Called when the game starts or when spawned
void UTopDownCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	const FRotator InitialRotation = GetComponentRotation();
	InitialFOV = FieldOfView;
	
	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);


	RecordScreenSize(GetOwner()->GetInstigatorController());

	if (ATetherCharacter* TetherCharacter = Cast<ATetherCharacter>(GetOwner()))
	{
		Subject = TetherCharacter;
		
		TetherCharacter->OnPossessedDelegate().AddWeakLambda(this, [this](AController* Controller)
		{
			RecordScreenSize(Controller);
		});
	}
	SetWorldRotation(InitialRotation);
	ResetLocation();
}


// Called every frame
void UTopDownCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bLockRotation)
	{
		SetWorldRotation(CalcDeltaRotation(DeltaTime));
	}
	if (!bLockZoom)
	{
		SubjectDistance = CalcDeltaZoom(DeltaTime);
	}
	FVector Location = CalcDeltaLocation(DeltaTime);
	if (bUseSpringArm)
	{
		if (GetWorld() && Subject)
		{
			FHitResult LineTraceHitResult;
			FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
			QueryParams.AddIgnoredActor(Subject);
			const FVector CameraFacing = (SubjectLocation - Location).GetSafeNormal();
			if (GetWorld()->LineTraceSingleByChannel(LineTraceHitResult,
				SubjectLocation + CameraFacing * -20.0f,
				Location,
				ECollisionChannel::ECC_Camera,
				QueryParams))
			{
				Location = LineTraceHitResult.Location;
			}
		}
	}
	SetWorldLocation(Location);
}


void UTopDownCameraComponent::AddCameraRotation(const FRotator Rotator)
{
	PendingRotations += Rotator;
}


FVector UTopDownCameraComponent::CalcDeltaLocation(const float DeltaTime)
{
	SubjectLocation = Subject->GetActorLocation() + Subject->EyeHeight * FVector::UpVector;
	const FVector Velocity = Subject->IsSuspended() ? FVector::ZeroVector : Subject->GetVelocity();
	DesiredFocalPoint =  SubjectLocation + (Velocity * TrackAnticipationTime).GetClampedToMaxSize(MaxAnticipation);
	FocalPoint = FMath::VInterpTo(FocalPoint, DesiredFocalPoint, DeltaTime, TrackingSpeed);

	return FocalPoint + GetWorldOffset();
}


float UTopDownCameraComponent::CalcDeltaZoom(const float DeltaTime)
{
	DesiredSubjectDistance = GetMinimumDistance();
	return FMath::Clamp(FMath::FInterpTo(SubjectDistance, DesiredSubjectDistance, DeltaTime, ZoomSpeed), MinimumSubjectDistance, MaximumSubjectDistance);
}


FRotator UTopDownCameraComponent::CalcDeltaRotation(const float DeltaTime)
{
	FRotator NewRotation = GetComponentRotation();
	RotationalVelocity = ConsumeRotations() * RotationSpeed;

	RotationalVelocity.Yaw *= XSensitivity * (bInvertCameraX ? -1 : 1);
	RotationalVelocity.Pitch *= YSensitivity * (bInvertCameraY ? 1 : -1);

	if (Subject->IsSuspended())
	{
		RotationalVelocity = FRotator::ZeroRotator;
	}
	
	NewRotation += RotationalVelocity * DeltaTime;

	const float DeltaRotation = FMath::FindDeltaAngleDegrees(GetComponentRotation().Yaw, Subject->GetActorRotation().Yaw);
	if (Subject && CopyRotationFactor > 0.0f && Subject->MovementComponent->bIsWalking && FMath::Abs(DeltaRotation) <= 160.0f)
	{
		NewRotation.Yaw += DeltaRotation * CopyRotationFactor * Subject->MovementComponent->InputFactor * DeltaTime;
	}	
	NewRotation.Pitch = FMath::ClampAngle(NewRotation.Pitch, MinPitch, MaxPitch);
	return NewRotation;
}


FVector2D UTopDownCameraComponent::ConvertVectorToViewSpace(const FVector& WorldLocation) const
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwner()->GetInstigatorController()))
	{
		FVector2D ScreenLocation;
		PlayerController->ProjectWorldLocationToScreen(WorldLocation, ScreenLocation);
		return ScreenLocation - ScreenSize / 2;
	}
	return FVector2D::ZeroVector;
}


float UTopDownCameraComponent::GetMinimumDistance()
{
	return 100.0f;
}


FVector UTopDownCameraComponent::GetWorldOffset() const
{
	return GetForwardVector() * -SubjectDistance;
}

FRotator UTopDownCameraComponent::ConsumeRotations()
{
	const FRotator Rotation = PendingRotations;	
	PendingRotations = FRotator::ZeroRotator;
	return Rotation;
}


void UTopDownCameraComponent::RecordScreenSize(AController* Controller)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		int ScreenSizeX, ScreenSizeY;
		PlayerController->GetViewportSize(ScreenSizeX, ScreenSizeY);
		if (PlayerController->GetSplitscreenPlayerCount() >= 2)
		{
			FieldOfView = InitialFOV / 2.0f;
		}
		else
		{
			FieldOfView = InitialFOV;
		}
		ScreenSize = FVector2D(ScreenSizeX, ScreenSizeY);
	}
}


void UTopDownCameraComponent::ResetLocation()
{
	if (!Subject)
	{
		return;
	}
	SubjectLocation = Subject->GetActorLocation() + Subject->EyeHeight * FVector::UpVector;
	DesiredFocalPoint = SubjectLocation + (Subject->GetVelocity() * TrackAnticipationTime).GetClampedToMaxSize(MaxAnticipation);
	FocalPoint = DesiredFocalPoint;

	DesiredSubjectDistance = SubjectDistance;
	
	SetWorldLocation(DesiredFocalPoint + DesiredSubjectDistance * -GetForwardVector());
}
