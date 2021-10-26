// Copyright (c) 2021 Spencer Melnick

#include "TetherPrimaryGameMode.h"

#include "EngineUtils.h"
#include "Tether/Character/TetherCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "TetherPrimaryGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Tether/Tether.h"
#include "Tether/Controller/TetherPlayerController.h"
#include "Tether/Core/TetherUtils.h"
#include "Tether/Gameplay/Beam/BeamController.h"
#include "Tether/Core/Suspendable.h"

ATetherPrimaryGameMode::ATetherPrimaryGameMode()
{
	DefaultPawnClass = ATetherCharacter::StaticClass();
	PlayerControllerClass = ATetherPlayerController::StaticClass();
	GameStateClass = ATetherPrimaryGameState::StaticClass();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}


// Actor overrides

void ATetherPrimaryGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ATetherPrimaryGameMode::StartPlay()
{
	UWorld* World = GetWorld();
	if (ensure(World))
	{
		if (BeamControllerClass)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.ObjectFlags |= RF_Transient;
			
			BeamController = World->SpawnActor<ABeamController>(BeamControllerClass,
				FVector::ZeroVector, FRotator::ZeroRotator,
				SpawnParameters);
		}
		else
		{
			UE_LOG(LogTetherGame, Warning, TEXT("TetherPrimaryGameMode::BeginPlay - no beam controller class specified"));
		}
	}
	Super::StartPlay();
}


void ATetherPrimaryGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (ensure(World))
	{
		ATetherPrimaryGameState* TetherGameState = GetGameState<ATetherPrimaryGameState>();
		if (ensure(TetherGameState))
		{
			TetherGameState->SetGlobalHealth(MaxHealth);
			TetherGameState->SetGamePhase(ETetherGamePhase::Warmup);
		
			FTimerHandle WarmupTimerHandle;
			World->GetTimerManager().SetTimer(WarmupTimerHandle, FTimerDelegate::CreateWeakLambda(TetherGameState, [this, TetherGameState]()
			{
				TetherGameState->SetGamePhase(ETetherGamePhase::Playing);
			}), WarmupTime, false);
		}
	}
	ObstacleVolume = TetherUtils::FindFirstActorWithTag<AVolume>(this, TEXT("ObstacleVolume"));
	if (!ObstacleVolume)
	{
		UE_LOG(LogTetherGame, Warning, TEXT("TetherPrimaryGameMode::BeginPlay - No obstacle volume found!"));
	}
}


float ATetherPrimaryGameMode::GetBaseObstacleSpeed(float GamePhaseTime) const
{
	if (ObstacleSpeedCurve)
	{
		return ObstacleSpeedMultiplier * ObstacleSpeedCurve->GetFloatValue(GamePhaseTime);
	}

	UE_LOG(LogTetherGame, Warning, TEXT("TetherPrimaryGameMode::GetBaseObstacleSpeed failed - no speed curve specified"));
	return 0.f;
}
