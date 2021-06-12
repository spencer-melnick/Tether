// Copyright Epic Games, Inc. All Rights Reserved.


#include "TetherGameModeBase.h"

#include "Character/TetherCharacter.h"
#include "Edison/EdisonActor.h"
#include "NiagaraFunctionLibrary.h"

ATetherGameModeBase::ATetherGameModeBase()
{
	DefaultPawnClass = ATetherCharacter::StaticClass();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}


// Actor overrides

void ATetherGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckAllTethers();
}

void ATetherGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SpawnEdisons();
}


// Tether functions

void ATetherGameModeBase::SpawnEdisons()
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

void ATetherGameModeBase::CheckAllTethers()
{
	if (const UWorld* World = GetWorld())
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
						EdisonActor->SetEndpoints(FirstCharacter->GetTetherTargetLocation(), SecondCharacter->GetTetherTargetLocation());
					}
				}
			}
		}
	}
}

bool ATetherGameModeBase::AreCharactersTethered(const ATetherCharacter* FirstCharacter, const ATetherCharacter* SecondCharacter) const
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

