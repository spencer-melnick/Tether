// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "TetherPlayerController.generated.h"

/**
 * Main player controller for tether game
 */
UCLASS()
class ATetherPlayerController: public APlayerController
{
	GENERATED_BODY()

public:

	/** Invoked by the game mode to spawn the initial HUD widgets */
	void SpawnHUDWidgets();

	virtual void BeginPlay() override;

	// Accessors
	const TArray<TSubclassOf<UUserWidget>>& GetPlayerWidgetClasses() const { return PlayerWidgetClasses; }

	void SetPlayerSlot(const int SlotNumber);

	int GetPlayerSlot() const;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	int PlayerSlotNumber;

private:

	// HUD tracking
	bool bSpawnedHUDWidgets = false;
	
	// Editor properties

	/** Widgets to be spawned on this player controller regardless of game mode */
	UPROPERTY(EditDefaultsOnly, Category="HUD")
	TArray<TSubclassOf<UUserWidget>> PlayerWidgetClasses;
};
