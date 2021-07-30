// Copyright (c) 2021 Spencer Melnick

#include "TetherPrimaryGameMode.h"

#include "EngineUtils.h"
#include "Tether/Character/TetherCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "TetherPrimaryGameState.h"
#include "Tether/Tether.h"
#include "Tether/Controller/TetherPlayerController.h"
#include "Tether/Core/TetherUtils.h"
#include "Tether/Gameplay/BeamComponent.h"
#include "Tether/Gameplay/BeamController.h"

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

	UWorld* World = GetWorld();
	if (ensure(World))
	{
		ATetherPrimaryGameState* TetherGameState = GetGameState<ATetherPrimaryGameState>();
		if (ensure(TetherGameState))
		{
			TetherGameState->SetGlobalHealth(MaxHealth);
			TetherGameState->SetGamePhase(ETetherGamePhase::Warmup);
		
			FTimerHandle WarmupTimerHandle;
			World->GetTimerManager().SetTimer(WarmupTimerHandle, FTimerDelegate::CreateWeakLambda(TetherGameState, [TetherGameState]()
			{
				TetherGameState->SetGamePhase(ETetherGamePhase::Playing);
			}), WarmupTime, false);
		}
		
		if (BeamControllerClass)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.ObjectFlags |= RF_Transient;
			
			BeamController = World->SpawnActor<ABeamController>(BeamControllerClass,
				FVector::ZeroVector, FRotator::ZeroRotator,
				SpawnParameters);

			if (ensure(BeamController))
			{
				for (TActorIterator<AActor> Iterator(World); Iterator; ++Iterator)
				{
					if (Iterator->Implements<UBeamTarget>())
					{
						BeamController->AddBeamTarget(*Iterator);
					}
				}
			}
		}
		else
		{
			UE_LOG(LogTetherGame, Warning, TEXT("TetherPrimaryGameMode::BeginPlay - no beam controller class specified"));
		}
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
