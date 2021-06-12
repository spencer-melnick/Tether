// Copyright Epic Games, Inc. All Rights Reserved.


#include "TetherGameModeBase.h"

#include "Character/TetherCharacter.h"
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

	TetherEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TetherEffectSystem, FVector::ZeroVector);
	TetherEffectComponent->SetVisibility(false);
}


// Tether functions

void ATetherGameModeBase::CheckAllTethers()
{
	if (const UWorld* World = GetWorld())
	{
		TSet<const APlayerController*> AllTetheredPlayers;
		TArray<TPair<const APlayerController*, const APlayerController*>> Tethers;

		for (FConstPlayerControllerIterator FirstIterator = World->GetPlayerControllerIterator(); FirstIterator; ++FirstIterator)
		{
			const APlayerController* FirstPlayer = FirstIterator->Get();
			
			for (FConstPlayerControllerIterator SecondIterator = FirstIterator + 1; SecondIterator; ++SecondIterator)
			{
				const APlayerController* SecondPlayer = SecondIterator->Get();

				if (FirstPlayer && SecondPlayer && ArePlayersTethered(FirstPlayer, SecondPlayer))
				{
					AllTetheredPlayers.Add(FirstPlayer);
					AllTetheredPlayers.Add(SecondPlayer);
					Tethers.Emplace(FirstPlayer, SecondPlayer);
				}
			}
		}
	}
}

bool ATetherGameModeBase::ArePlayersTethered(const APlayerController* FirstPlayer, const APlayerController* SecondPlayer) const
{
	ensure(FirstPlayer && SecondPlayer);

	const ATetherCharacter* FirstCharacter = FirstPlayer ? FirstPlayer->GetPawn<ATetherCharacter>() : nullptr;
	const ATetherCharacter* SecondCharacter = SecondPlayer ? SecondPlayer->GetPawn<ATetherCharacter>() : nullptr;
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
			if (TetherEffectComponent)
			{
				TetherEffectComponent->SetVisibility(true);
				TetherEffectComponent->SetVectorParameter(TEXT("StartLocation"), FirstCharacter->GetTetherTargetLocation());
				TetherEffectComponent->SetVectorParameter(TEXT("EndLocation"), SecondCharacter->GetTetherTargetLocation());
            }
			
			return true;
		}
	}

	if (TetherEffectComponent)
	{
		TetherEffectComponent->SetVisibility(false);
	}

	return false;
}

