// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherBlueprintLibrary.h"

#include "GameFramework/GameStateBase.h"
#include "Tether/GameMode/TetherGameModeBase.h"


ATetherGameModeBase* UTetherBlueprintLibrary::GetGameModeCDO(const UObject* WorldContextObject)
{
	if (ensure(WorldContextObject))
	{
		const UWorld* World = WorldContextObject->GetWorld();
		const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;

		if (GameState)
		{
			return const_cast<ATetherGameModeBase*>(GameState->GetDefaultGameMode<ATetherGameModeBase>());
		}
	}

	return nullptr;
}
