// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherGameInstance.h"
#include "Tether/Tether.h"

void UTetherGameInstance::SetNumberOfPlayers(int32 NumPlayers)
{
	if (ensure(NumPlayers > 0))
	{
		UE_LOG(LogTetherGame, Display, TEXT("TetherGameInstance - setting number of players to %d"), NumPlayers);
		
		if (GetNumLocalPlayers() < NumPlayers)
		{
			while (GetNumLocalPlayers() < NumPlayers)
			{
				FString OutError;
				if (!CreateLocalPlayer(-1, OutError, true))
				{
					UE_LOG(LogTetherGame, Error, TEXT("TetherGameInstance::CreateLocalPlayer failed: %s"), *OutError);
					break;
				}
			}
		}
		else if (GetNumLocalPlayers() > NumPlayers)
		{
			while (GetNumLocalPlayers() > NumPlayers)
			{
				RemoveLocalPlayer(GetLocalPlayerByIndex(GetNumLocalPlayers() - 1));
			}
		}

		UE_LOG(LogTetherGame, Display, TEXT("TetherGameInstance - actual number of players is %d"), GetNumLocalPlayers());
	}
}

void UTetherGameInstance::OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld)
{
	Super::OnWorldChanged(OldWorld, NewWorld);

#if WITH_EDITOR
	if (!OldWorld && NewWorld && NewWorld->IsPlayInEditor())
	{
		bInStartingPIEWorld = true;
	}
	else
	{
		bInStartingPIEWorld = false;
	}
#endif
}
