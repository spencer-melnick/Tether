// Copyright (c) 2021 Spencer Melnick

#include "TetherPrimaryGameMode.h"

#include "EngineUtils.h"
#include "Tether/Character/TetherCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "TetherPrimaryGameState.h"
#include "Tether/Tether.h"
#include "Tether/Controller/TetherPlayerController.h"
#include "Tether/Core/TetherUtils.h"

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

void ATetherPrimaryGameMode::BeginPlay()
{
	Super::BeginPlay();

	ObstacleVolume = TetherUtils::FindFirstActorWithTag<AVolume>(this, TEXT("ObstacleVolume"));
	if (!ObstacleVolume)
	{
		UE_LOG(LogTetherGame, Warning, TEXT("TetherPrimaryGameMode::BeginPlay - No obstacle volume found!"));
	}

	const UWorld* World = GetWorld();
	ATetherPrimaryGameState* TetherGameState = GetGameState<ATetherPrimaryGameState>();
	if (ensure(World && TetherGameState))
	{
		TetherGameState->SetGlobalHealth(MaxHealth);
		TetherGameState->SetGamePhase(ETetherGamePhase::Warmup);
		
		FTimerHandle WarmupTimerHandle;
		World->GetTimerManager().SetTimer(WarmupTimerHandle, FTimerDelegate::CreateWeakLambda(TetherGameState, [TetherGameState]()
		{
			TetherGameState->SetGamePhase(ETetherGamePhase::Playing);
		}), WarmupTime, false);
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
