// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherFrontendGameMode.h"


ATetherFrontendGameMode::ATetherFrontendGameMode()
{
	bUseSplitscreen = false;
}

APawn* ATetherFrontendGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer,
                                                                           const FTransform& SpawnTransform)
{
	// We don't need a pawn in the frontend! Don't do anything here
	return nullptr;
}
