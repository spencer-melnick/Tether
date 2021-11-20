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
	UWorld* World = GetWorld();
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
					for (FActorIterator ActorIterator(World); ActorIterator; ++ActorIterator)
					{
						if (ISuspendable* Actor = Cast<ISuspendable>(*ActorIterator))
						{
							// Don't pause the player character
							if (APawn* Character = Cast<APawn>(Actor))
							{
								// TODO: update this array appropriately
								CharacterPawns.Add(Character);
								continue;
							}
							Actor->Suspend();
						}
					}
					
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
					SuspendActors();
					/* FTimerHandle ResetTimerHandle;
					GetWorld()->GetTimerManager().SetTimer(ResetTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
					{
						SetGlobalHealth(100.0f);
						ReloadActors();
						SetGamePhase(ETetherGamePhase::Playing);						
					}), 10.0f, false); */
					break;
				}
			default:
				{
					break;
				}
			}
	}
}


void ATetherPrimaryGameState::SoftRestart()
{
	ReloadActors();
	SetGlobalHealth(MaxGlobalHealth);
	SetGamePhase(ETetherGamePhase::Playing);
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


float ATetherPrimaryGameState::AddGlobalHealth(const float HealAmount)
{
	if (GamePhase == ETetherGamePhase::Playing)
	{
		if (HealAmount + GlobalHealth > MaxGlobalHealth)
		{
			const float LastHealth = GlobalHealth;
			SetGlobalHealth(MaxGlobalHealth);
			return MaxGlobalHealth - LastHealth;
		}
		SetGlobalHealth(GlobalHealth + HealAmount);
		return HealAmount;
	}
	return 0.0f;
}

void ATetherPrimaryGameState::AddScore(const int Points)
{
	Score += Points;
}

int ATetherPrimaryGameState::GetScore()
{
	return Score;
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


TArray<TWeakObjectPtr<APawn>> ATetherPrimaryGameState::GetCharacterPawns() const
{
	return CharacterPawns;
}


APawn* ATetherPrimaryGameState::GetClosestCharacterPawn(const FVector& Location)
{
	float MinDistance = -1.0f;
	APawn* Result = nullptr;
	for (TWeakObjectPtr<APawn> Character : CharacterPawns)
	{
		if (!Character.IsValid())
		{
			continue;
		}
		const float Distance = FVector::DistSquared(Location, Character->GetActorLocation()); 
		if (Distance < MinDistance || MinDistance < 0.0f)
		{
			MinDistance = Distance;
			Result = Character.Get();
		}
	}
	return Result;
}


APawn* ATetherPrimaryGameState::GetClosestCharacterPawn(const FVector& Location, APawn* IgnorePawn)
{
	float MinDistance = -1.0f;
	APawn* Result = nullptr;
	for (TWeakObjectPtr<APawn> Character : CharacterPawns)
	{
		if (!Character.IsValid() || Character == IgnorePawn)
		{
			continue;
		}
		const float Distance = FVector::DistSquared(Location, Character->GetActorLocation()); 
		if (Distance < MinDistance || MinDistance < 0.0f)
		{
			MinDistance = Distance;
			Result = Character.Get();
		}
	}
	return Result;
}


void ATetherPrimaryGameState::OnRep_GlobalHealth()
{
	OnGlobalHealthChanged.Broadcast(GlobalHealth);
}


void ATetherPrimaryGameState::SuspendActors()
{
	bIsSuspended = true;
	if (UWorld* World = GetWorld())
	{
		for (FActorIterator ActorIterator(World); ActorIterator; ++ActorIterator)
		{
			if (ISuspendable* Actor = Cast<ISuspendable>(*ActorIterator))
			{
				Actor->Suspend();
			}
		}
	}
}

void ATetherPrimaryGameState::UnsuspendActors()
{
	bIsSuspended = false;
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