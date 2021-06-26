// Copyright 2021 Spencer Melnick, Stephen Melnick

#include "TetherGameModeBase.h"
#include "Tether/Controller/TetherPlayerController.h"

ATetherGameModeBase::ATetherGameModeBase()
{
	PlayerControllerClass = ATetherPlayerController::StaticClass();
}

void ATetherGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// Disable splitscreen if this mode should not have splitscreen enabled
	if (ensure(GEngine && GEngine->GameViewport))
	{
		GEngine->GameViewport->SetForceDisableSplitscreen(!bUseSplitscreen);
	}
}
