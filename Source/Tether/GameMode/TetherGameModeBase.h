// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NiagaraComponent.h"
#include "TetherGameModeBase.generated.h"


class AEdisonActor;
class ATetherCharacter;

/**
 * 
 */
UCLASS()
class TETHER_API ATetherGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	ATetherGameModeBase();


	// Actor overrides

	virtual void BeginPlay() override;


	// Accessors


	UFUNCTION(BlueprintPure, Category="Splitscreen")
	bool ShouldUseSplitscreen() const { return bUseSplitscreen; }

	UFUNCTION(BlueprintPure, Category="UI")
	const TArray<TSubclassOf<UUserWidget>>& GetHUDWidgetClasses() const { return HUDWidgetClasses; }


protected:

	// UI Settings

	/** If this gamemode should have splitscreen enabled */
	UPROPERTY(EditDefaultsOnly, Category="Splitscreen")
	bool bUseSplitscreen = false;

	/**
	 * Widgets that should be spawned by any player controllers in this gamemode. Additional widgets can be specified
	 * per player controller
	 */
	UPROPERTY(EditDefaultsOnly, Category="HUD")
	TArray<TSubclassOf<UUserWidget>> HUDWidgetClasses;
};
