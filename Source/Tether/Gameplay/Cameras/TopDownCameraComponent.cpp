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

	InitialFOV = FieldOfView;
	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);
	Subjects = GetSubjectActors();

	RecordScreenSize(GetOwner()->GetInstigatorController());

	if (ATetherCharacter* TetherCharacter = Cast<ATetherCharacter>(GetOwner()))
	{
		TetherCharacter->OnPossessedDelegate().AddWeakLambda(this, [this](AController* Controller)
		{
			RecordScreenSize(Controller);
		});
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
	FVector Location = CalcDeltaLocation(DeltaTime);
	if (bUseSpringArm)
	{
		if (UWorld* World = GetWorld())
		{
			FHitResult LineTraceHitResult;
			FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
			for (AActor* Subject : Subjects)
			{
				if (Subject)
				{
					QueryParams.AddIgnoredActors(Subjects);
				}
				else
				{
					Subjects.Remove(Subject);
				}
			}
			const FVector CameraFacing = (Subjects[0]->GetActorLocation() - Location).GetSafeNormal();
			if (World->LineTraceSingleByChannel(LineTraceHitResult,
				Subjects[0]->GetActorLocation() + CameraFacing * -20.0f,
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
	DesiredFocalPoint = AverageLocationOfTargets(Subjects);
	if (bKeepSubjectFramed)
	{
		if (SubjectLocations[0].SizeSquared() > 1)
		{
			FocalPoint = DesiredFocalPoint;
		}	
	}
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

	NewRotation += RotationalVelocity * DeltaTime;

	if (ATetherCharacter* Player = Cast<ATetherCharacter>(Subjects[0]))
	{
		const float DeltaRotation = FMath::FindDeltaAngleDegrees(GetComponentRotation().Yaw, Player->GetActorRotation().Yaw);
		if (CopyRotationFactor > 0.0f && Player->MovementComponent->bIsWalking && FMath::Abs(DeltaRotation) <= 160.0f)
		{
			NewRotation.Yaw += DeltaRotation * CopyRotationFactor * DeltaTime;
		}
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


TArray<AActor*> UTopDownCameraComponent::GetSubjectActors() const
{
	TArray<AActor*> OutputArray;

	if (bTrackAllPlayers)
	{
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
	}
	if (bTrackParent)
	{
		AActor* Parent = GetOwner();
		if (!OutputArray.Contains(Parent))
		{
			OutputArray.Add(Parent);
		}
	}
	return OutputArray;
}


FVector UTopDownCameraComponent::AverageLocationOfTargets(TArray<AActor*> Targets) const
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
				FVector AnticipationOffset = Target->GetVelocity() * TrackAnticipationTime;
				Average += AnticipationOffset.GetClampedToMaxSize(MaxAnticipation);
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
