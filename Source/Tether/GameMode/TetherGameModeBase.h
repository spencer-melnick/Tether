// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NiagaraComponent.h"
#include "GameFramework/PlayerStart.h"

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

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
			

	// Accessors

	UFUNCTION(BlueprintPure, Category="Splitscreen")
	bool ShouldUseSplitscreen() const { return bUseSplitscreen; }

	UFUNCTION(BlueprintPure, Category="UI")
	const TArray<TSubclassOf<UUserWidget>>& GetPlayerWidgetClasses() const { return PlayerWidgetClasses; }

	UFUNCTION(BlueprintPure, Category="UI")
	const TArray<TSubclassOf<UUserWidget>>& GetGameWidgetClasses() const { return GameWidgetClasses; }

	bool IsPlayingInEditor() const;

protected:

	/** Creates HUD widgets for all player controllers */
	UFUNCTION(BlueprintCallable)
	void SpawnHUDWidgets();

#if WITH_EDITOR
	/** Returns true if this game mode should spawn PIE players when it is the first loaded gamemode */
	virtual bool ShouldSpawnPIEPlayers() const { return false; }
#endif

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
