// Copyright (c) 2021 Spencer Melnick

#include "TetherPrimaryGameMode.h"

#include "EngineUtils.h"
#include "Tether/Character/TetherCharacter.h"
#include "Tether/Edison/EdisonActor.h"
#include "NiagaraFunctionLibrary.h"
#include "TetherPrimaryGameState.h"
#include "Tether/Tether.h"

ATetherPrimaryGameMode::ATetherPrimaryGameMode()
{
	DefaultPawnClass = ATetherCharacter::StaticClass();
	GameStateClass = ATetherPrimaryGameState::StaticClass();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}


// Actor overrides

void ATetherPrimaryGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckAllTethers(DeltaSeconds);
}

void ATetherPrimaryGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnEdisons();

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AVolume> Iterator(World); Iterator; ++Iterator)
		{
			if (Iterator->ActorHasTag(TEXT("ObstacleVolume")))
			{
				ObstacleVolume = *Iterator;
				break;
			}
		}

		if (!ObstacleVolume)
		{
			UE_LOG(LogTetherGame, Warning, TEXT("No obstacle volume found!"));
		}

		if (ATetherPrimaryGameState* TetherGameState = GetGameState<ATetherPrimaryGameState>())
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
}

float ATetherPrimaryGameMode::GetObstacleSpeed() const
{
	const UWorld* World = GetWorld();
	if (World && ObstacleSpeedCurve)
	{
		return BaseObstacleSpeed * ObstacleSpeedCurve->GetFloatValue(World->GetTimeSeconds());
	}

	return BaseObstacleSpeed;
}


// Tether functions

void ATetherPrimaryGameMode::SpawnEdisons()
{
	UWorld* World = GetWorld();
	if (World && TetherEffectClass)
	{
		for (FConstPlayerControllerIterator FirstIterator = World->GetPlayerControllerIterator(); FirstIterator; ++FirstIterator)
		{
			const APlayerController* FirstPlayer = FirstIterator->Get();
			const ATetherCharacter* FirstCharacter = FirstPlayer ? FirstPlayer->GetPawn<ATetherCharacter>() : nullptr;

			EdisonMap.Emplace(FirstPlayer);
			
			for (FConstPlayerControllerIterator SecondIterator = FirstIterator + 1; SecondIterator; ++SecondIterator)
			{
				const APlayerController* SecondPlayer = SecondIterator->Get();
				const ATetherCharacter* SecondCharacter = SecondPlayer ? SecondPlayer->GetPawn<ATetherCharacter>() : nullptr;

				if (FirstCharacter && SecondCharacter)
				{
					AEdisonActor* NewEdison = World->SpawnActor<AEdisonActor>(TetherEffectClass);
					Edisons.Add(NewEdison);
					EdisonMap[FirstPlayer].Emplace(SecondPlayer, NewEdison);
				}
			}
		}
	}
}

void ATetherPrimaryGameMode::CheckAllTethers(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	ATetherPrimaryGameState* TetherGameState = GetGameState<ATetherPrimaryGameState>();
	if (World && TetherGameState && !bHaveTethersExpired)
	{
		TSet<const APlayerController*> AllTetheredPlayers;
		TArray<TPair<const APlayerController*, const APlayerController*>> Tethers;

		for (FConstPlayerControllerIterator FirstIterator = World->GetPlayerControllerIterator(); FirstIterator; ++FirstIterator)
		{
			const APlayerController* FirstPlayer = FirstIterator->Get();
			const ATetherCharacter* FirstCharacter = FirstPlayer ? FirstPlayer->GetPawn<ATetherCharacter>() : nullptr;
			
			for (FConstPlayerControllerIterator SecondIterator = FirstIterator + 1; SecondIterator; ++SecondIterator)
			{
				const APlayerController* SecondPlayer = SecondIterator->Get();
				const ATetherCharacter* SecondCharacter = SecondPlayer ? SecondPlayer->GetPawn<ATetherCharacter>() : nullptr;
				
				AEdisonActor* EdisonActor = EdisonMap[FirstPlayer][SecondPlayer];
				bool bAreCharactersTethered = false;
				
				if (FirstCharacter && SecondCharacter)
				{
					bAreCharactersTethered = AreCharactersTethered(FirstCharacter, SecondCharacter);
					if (bAreCharactersTethered)
					{
						AllTetheredPlayers.Add(FirstPlayer);
						AllTetheredPlayers.Add(SecondPlayer);
						Tethers.Emplace(FirstPlayer, SecondPlayer);
					}
				}

				if (EdisonActor)
				{
					EdisonActor->SetTetherVisibility(bAreCharactersTethered);

					if (FirstCharacter && SecondCharacter)
					{
						EdisonActor->SetEndpoints(FirstCharacter->GetTetherEffectLocation(), SecondCharacter->GetTetherEffectLocation());
					}
				}
			}
		}

		const bool bNewArePlayersTethered = AllTetheredPlayers.Num() >= World->GetNumPlayerControllers();
		if (bNewArePlayersTethered != bArePlayersTethered)
		{
			if (!bNewArePlayersTethered)
			{
				LastTetheredTime = World->GetTimeSeconds();
			}
			else
			{
				OnUntetheredTimeElapsed.Broadcast(0.f);
			}

			bArePlayersTethered = bNewArePlayersTethered;
			OnTetheredChanged.Broadcast(bArePlayersTethered);
		}

		if (TetherGameState->GetCurrentGamePhase() == ETetherGamePhase::Playing)
		{
			if (!bArePlayersTethered)
			{
				TetherGameState->SetGlobalHealth(TetherGameState->GetGlobalHealth() - DamagePerSecond * DeltaSeconds);

				if (TetherGameState->GetGlobalHealth() <= 0.f)
				{
					TetherGameState->SetGlobalHealth(0.f);
					TetherGameState->SetGamePhase(ETetherGamePhase::Ending);
				}
			}
			else
			{
				TetherGameState->SetGlobalHealth(MaxHealth);
			}
		}
	}
}

bool ATetherPrimaryGameMode::AreCharactersTethered(const ATetherCharacter* FirstCharacter, const ATetherCharacter* SecondCharacter) const
{
	const UWorld* World = GetWorld();
	
	if (World && FirstCharacter && SecondCharacter)
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(FirstCharacter);

#if !UE_BUILD_SHIPPING
		QueryParams.bDebugQuery = true;
		QueryParams.TraceTag = TEXT("TetherTrace");
#endif
		
		const bool bHitAnything = World->LineTraceSingleByChannel(HitResult,
			FirstCharacter->GetTetherTargetLocation(),
			SecondCharacter->GetTetherTargetLocation(),
			TetherTraceChannel, QueryParams);

		if (!bHitAnything || HitResult.GetActor() == SecondCharacter)
		{
			return true;
		}
	}

	return false;
}
