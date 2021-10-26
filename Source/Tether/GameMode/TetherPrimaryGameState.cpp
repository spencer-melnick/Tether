// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherPrimaryGameState.h"

#include "EngineUtils.h"
#include "TetherPrimaryGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Tether/Tether.h"
#include "Tether/Character/TetherCharacter.h"
#include "Tether/Core/Suspendable.h"

void ATetherPrimaryGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATetherPrimaryGameState, GamePhase);
}


void ATetherPrimaryGameState::SetGamePhase(ETetherGamePhase NewPhase)
{
	const UWorld* World = GetWorld();
	if (ensureAlways(World))
	{
		PhaseStartTime = World->GetTimeSeconds();
		GamePhase = NewPhase;
		OnGamePhaseChanged.Broadcast(GamePhase);

		UE_LOG(LogTetherGame, Display, TEXT("Setting game phase to %s"),
			*StaticEnum<ETetherGamePhase>()->GetValueAsString(GamePhase));

		switch(NewPhase)
		{
			case ETetherGamePhase::Warmup:
				{
					SuspendActors();
					
					CacheActorsInitialState();
					break;
				}
			case ETetherGamePhase::Playing:
				{
					UnsuspendActors();
					break;
				}
			case ETetherGamePhase::Ending:
				{
					ReloadActors();
					break;
				}
			default:
				{
					break;
				}
			}
	}
}


void ATetherPrimaryGameState::SetGlobalHealth(float NewGlobalHealth)
{
	GlobalHealth = NewGlobalHealth;

	// OnRep will never fire on the server so manually trigger it
	if (GetNetMode() < NM_Client)
	{
		OnRep_GlobalHealth();
	}
	if (GlobalHealth <= 0.0f)
	{
		const UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &ATetherPrimaryGameState::SetGamePhase, ETetherGamePhase::Ending));
		}
	}
}


float ATetherPrimaryGameState::SubtractGlobalHealth(const float DamageAmount)
{
	if (GamePhase == ETetherGamePhase::Playing)
	{
		if (DamageAmount >= GlobalHealth)
		{
			const float LastHealth = GlobalHealth;
			SetGlobalHealth(0.0f);
			return LastHealth;
		}
		SetGlobalHealth(GlobalHealth - DamageAmount);
		return DamageAmount;
	}
	return 0.0f;
}


float ATetherPrimaryGameState::GetTimeInCurrentPhase() const
{
	const UWorld* World = GetWorld();
	if (ensureAlways(World))
	{
		return World->GetTimeSeconds() - PhaseStartTime;
	}

	return 0.f;
}

float ATetherPrimaryGameState::GetBaseObstacleSpeed() const
{
	const ATetherPrimaryGameMode* GameMode = GetDefaultGameMode<ATetherPrimaryGameMode>();

	if (GamePhase == ETetherGamePhase::Playing && GameMode)
	{
		return GameMode->GetBaseObstacleSpeed(GetTimeInCurrentPhase());
	}

	return 0.f;
}

void ATetherPrimaryGameState::OnRep_GlobalHealth()
{
	OnGlobalHealthChanged.Broadcast(GlobalHealth);
}


void ATetherPrimaryGameState::SuspendActors()
{
	if (UWorld* World = GetWorld())
	{
		for (FActorIterator ActorIterator(World); ActorIterator; ++ActorIterator)
		{
			if (ISuspendable* Actor = Cast<ISuspendable>(*ActorIterator))
			{
				if (ActorIterator->IsA(ATetherCharacter::StaticClass()))
				{
					continue;
				}
				Actor->Suspend();
			}
		}
	}
}

void ATetherPrimaryGameState::UnsuspendActors()
{
	if (UWorld* World = GetWorld())
	{
		for (FActorIterator ActorIterator(World); ActorIterator; ++ActorIterator)
		{
			if (ISuspendable* Actor = Cast<ISuspendable>(*ActorIterator))
			{
				Actor->Unsuspend();
			}
		}
	}
}

void ATetherPrimaryGameState::CacheActorsInitialState()
{
	if (UWorld* World = GetWorld())
	{
		for (FActorIterator ActorIterator(World); ActorIterator; ++ActorIterator)
		{
			if (ISuspendable* Actor = Cast<ISuspendable>(*ActorIterator))
			{
				Actor->CacheInitialState();
			}
		}
	}
}

void ATetherPrimaryGameState::ReloadActors()
{
	if (UWorld* World = GetWorld())
	{
		for (FActorIterator ActorIterator(World); ActorIterator; ++ActorIterator)
		{
			if (ISuspendable* Actor = Cast<ISuspendable>(*ActorIterator))
			{
				Actor->Reload();
			}
		}
	}
}