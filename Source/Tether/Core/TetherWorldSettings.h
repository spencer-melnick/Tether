// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "TetherWorldSettings.generated.h"

class APlayerStart;
/**
 * 
 */
UCLASS()
class TETHER_API ATetherWorldSettings : public AWorldSettings
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable)
	APlayerStart* GetPlayerStart(const int Index);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spawning")
	TArray<APlayerStart*> DefaultPlayerStarts;
};
