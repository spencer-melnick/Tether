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

	virtual void BeginPlay() override;
	virtual void ReceivedPlayer() override;

	template <typename T>
	T* GetGameModeCDO() const
	{
		const UWorld* World = GetWorld();
		const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
		const TSubclassOf<AGameModeBase>& GameModeClass = GameState ? GameState->GameModeClass : nullptr;
		if (GameModeClass->IsChildOf<T>())
		{
			return GameModeClass->GetDefaultObject<T>();
		}

		return nullptr;
	}


	// Accessors
	const TArray<TSubclassOf<UUserWidget>>& GetHUDWidgetClasses() const { return HUDWidgetClasses; }


protected:

	virtual void SpawnHUDWidgets();


private:

	// HUD tracking
	bool bSpawnedHUDWidgets = false;

	
	// Editor properties

	// Widgets to be spawned on the player HUD regardless of game mode
	UPROPERTY(EditDefaultsOnly, Category="HUD")
	TArray<TSubclassOf<UUserWidget>> HUDWidgetClasses;
};
