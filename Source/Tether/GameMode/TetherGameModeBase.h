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
	const TArray<TSubclassOf<UUserWidget>>& GetPlayerWidgetClasses() const { return PlayerWidgetClasses; }

	UFUNCTION(BlueprintPure, Category="UI")
	const TArray<TSubclassOf<UUserWidget>>& GetGameWidgetClasses() const { return GameWidgetClasses; }

	/** Returns the default game mode object. Useful on clients when the live game mode is not available */
	static const ATetherGameModeBase* GetGameModeCDO(const UObject* WorldContextObject);

	bool IsPlayingInEditor() const;

protected:

	/** Creates HUD widgets for all player controllers */
	UFUNCTION(BlueprintCallable)
	void SpawnHUDWidgets();
	

	// UI Settings

	/** If this gamemode should have splitscreen enabled */
	UPROPERTY(EditDefaultsOnly, Category="Splitscreen")
	bool bUseSplitscreen = false;

	/** Widgets that should be spawned each player controller in this gamemode */
	UPROPERTY(EditDefaultsOnly, Category="HUD")
	TArray<TSubclassOf<UUserWidget>> PlayerWidgetClasses;

	/** Widgets that should be spawned across the entire screen in this gamemode */
	UPROPERTY(EditDefaultsOnly, Category="HUD")
	TArray<TSubclassOf<UUserWidget>> GameWidgetClasses;

private:

#if WITH_EDITOR
	/** Spawns additional PIE players according to Tether Developer Settings */
	void SpawnPIEPlayers();
#endif
	
};
