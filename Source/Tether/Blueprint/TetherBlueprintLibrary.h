// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "TetherBlueprintLibrary.generated.h"

class ATetherGameModeBase;


UCLASS()
class UTetherBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category="Tether", meta=(WorldContext="WorldContextObject"))
	static ATetherGameModeBase* GetGameModeCDO(const UObject* WorldContextObject);
};
