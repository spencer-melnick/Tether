// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "TopDownCameraComponent.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

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
	
	DesiredFocalPoint = AverageLocationOfTargets(Subjects);
	DesiredSubjectDistance = GetMinimumDistance();
	
	FocalPoint = DesiredFocalPoint;
	SubjectDistance = DesiredSubjectDistance;
}


// Called every frame
void UTopDownCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	DesiredFocalPoint = AverageLocationOfTargets(Subjects);
	DesiredSubjectDistance = GetMinimumDistance();

	FocalPoint = FMath::VInterpTo(FocalPoint, DesiredFocalPoint, DeltaTime, ZoomSpeed);
	SubjectDistance = FMath::Max(FMath::FInterpTo(SubjectDistance, DesiredSubjectDistance, DeltaTime, ZoomSpeed), MinimumSubjectDistance);
	
	SetWorldLocation(FocalPoint + GetWorldOffset());
}


FVector UTopDownCameraComponent::ConvertVectorToViewSpace(const FVector& WorldLocation) const
{
	FVector Offset = DesiredFocalPoint - WorldLocation;

	const float X = FVector::DotProduct(Offset, GetRightVector());
	const float Y = FVector::DotProduct(Offset, GetForwardVector());
	return FVector(X, Y, 0.0f);
}


FVector2D UTopDownCameraComponent::CalcMinScreenBounds() const
{
	float MaxX = 0, MaxY = 0;
	
	for (AActor* Subject : Subjects)
	{
		FVector ScreenSpace = ConvertVectorToViewSpace(Subject->GetActorLocation());
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
	
	for (AActor* Target : Targets)
	{
		Average += (Target->GetActorLocation());
	}
	return Targets.Num() == 0 ? FVector::ZeroVector : Average / Targets.Num();
}


float UTopDownCameraComponent::GetMinimumDistance() const
{
	return CalcMinScreenBounds().X / (2 * FMath::Tan(FMath::DegreesToRadians(FieldOfView / 2)));
}


FVector UTopDownCameraComponent::GetWorldOffset() const
{
	return GetForwardVector() * -SubjectDistance * 1.6;
}
