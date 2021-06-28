// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "TetherDeveloperSettings.generated.h"

UCLASS(config = EditorPerProjectUserSettings, defaultconfig)
class UTetherDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	/** Number of players to use when testing via play in editor */
	UPROPERTY(config, EditAnywhere, Category = "Multiplayer", meta=(DisplayName="Number of PIE Players", ClampMin=1, ClampMax=4))
	int32 NumPIEPlayers = 2;
};
