// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "TopDownCameraComponent.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
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

	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);
	
	Subjects = GetPlayerControlledActors();
		
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			int ScreenSizeX, ScreenSizeY;
			PlayerController->GetViewportSize(ScreenSizeX, ScreenSizeY);
			ScreenSize = FVector2D(ScreenSizeX, ScreenSizeY);
		}
	}

	SubjectDistance = CalcDeltaZoom(0.0f);
	RotationalVelocity = CalcDeltaRotation(0.0f);
	
	SetWorldLocation(CalcDeltaLocation(0.0f));
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
	SetWorldLocation(CalcDeltaLocation(DeltaTime));
}


void UTopDownCameraComponent::AddCameraRotation(const FRotator Rotator)
{
	PendingRotations += Rotator;
}


FVector UTopDownCameraComponent::CalcDeltaLocation(const float DeltaTime)
{
	DesiredFocalPoint = AverageLocationOfTargets(Subjects);
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
	RotationalVelocity = ConsumeRotations() * RotationSensitivity;

	NewRotation += RotationalVelocity;
	NewRotation.Pitch = FMath::ClampAngle(NewRotation.Pitch, MinPitch, MaxPitch);
	return NewRotation;
}


FVector2D UTopDownCameraComponent::ConvertVectorToViewSpace(const FVector& WorldLocation) const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* Player = World->GetFirstPlayerController())
		{
			FVector2D ScreenLocation;
			Player->ProjectWorldLocationToScreen(WorldLocation, ScreenLocation);
			return ScreenLocation - ScreenSize / 2;
		}
	}
	return FVector2D::ZeroVector;
}


FVector2D UTopDownCameraComponent::CalcSubjectScreenLocations()
{
	float MaxX = 0, MaxY = 0;

	const int SubjectCount = Subjects.Num();
	if (SubjectCount != SubjectLocations.Num())
	{
		SubjectLocations.SetNum(SubjectCount);
	}
	
	for (int i = 0; i < Subjects.Num(); i++)
	{
		if (Subjects[i])
		{
			FVector2D ScreenSpace = ConvertVectorToViewSpace(Subjects[i]->GetActorLocation());
			SubjectLocations[i] = ScreenSpace * 2 / ScreenSize;
			
			ScreenSpace = ScreenSpace.GetAbs();
			
			if (ScreenSpace.X > MaxX)
			{
				MaxX = ScreenSpace.X;
			}

			if (ScreenSpace.Y > MaxY)
			{
				MaxY = ScreenSpace.Y;
			}
		}
	}
	if (MaxX < MaxY * AspectRatio)
	{
		MaxX = MaxY * AspectRatio;
	}
	return FVector2D(MaxX * 2, MaxY * 2);
}


TArray<AActor*> UTopDownCameraComponent::GetPlayerControlledActors() const
{
	TArray<AActor*> OutputArray;
	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			TArray<APlayerState*> PlayerStates = GameState->PlayerArray;
			for (APlayerState* PlayerState : PlayerStates)
			{
				AActor* PlayerControlledActor = PlayerState->GetPawn();
				if (PlayerControlledActor)
				{
					OutputArray.Add(PlayerControlledActor);
				}
			}
		}
	}
	return OutputArray;
}


FVector UTopDownCameraComponent::AverageLocationOfTargets(TArray<AActor*> Targets)
{
	FVector Average = FVector::ZeroVector;
	int Count = Targets.Num();
	
	for (AActor* Target : Targets)
	{
		if (Target)
		{
			Average += Target->GetActorLocation();

			if (bTrackAheadOfMotion)
			{
				Average += Target->GetVelocity() * TrackAnticipationTime;
			}
		}
		else
		{
			Count --;
		}
	}
	return Count == 0 ? FVector::ZeroVector : Average / Count;
}


float UTopDownCameraComponent::GetMinimumDistance()
{
	return CalcSubjectScreenLocations().X / (2 * FMath::Tan(FMath::DegreesToRadians(FieldOfView / 2)));
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
