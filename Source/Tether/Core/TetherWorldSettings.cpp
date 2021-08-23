// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "TetherWorldSettings.h"
#include "Tether/Tether.h"

APlayerStart* ATetherWorldSettings::GetPlayerStart(const int Index)
{
	if (DefaultPlayerStarts.IsValidIndex(Index))
	{
		return DefaultPlayerStarts[Index];
	}
	else
	{
		UE_LOG(LogTetherGame, Warning, TEXT("Attempted to access a PlayerStart from WorldSettings, but index %i was invalid."), Index);
		return nullptr;
	}
}
