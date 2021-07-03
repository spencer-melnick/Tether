// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "TetherGameModeBase.h"
#include "TetherFrontendGameMode.generated.h"

UCLASS()
class ATetherFrontendGameMode : public ATetherGameModeBase
{
	GENERATED_BODY()

public:

	ATetherFrontendGameMode();
	

	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
};
