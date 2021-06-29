// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherPrimaryGameState.h"
#include "Net/UnrealNetwork.h"
#include "Tether/Tether.h"

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

void ATetherPrimaryGameState::OnRep_GlobalHealth()
{
	OnGlobalHealthChanged.Broadcast(GlobalHealth);
}
