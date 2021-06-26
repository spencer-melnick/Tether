// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Tether/GameMode/TetherGameModeBase.h"

void ATetherPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!bSpawnedHUDWidgets && GetLocalPlayer())
	{
		SpawnHUDWidgets();
		bSpawnedHUDWidgets = true;
	}
}

void ATetherPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (!bSpawnedHUDWidgets)
	{
		SpawnHUDWidgets();
		bSpawnedHUDWidgets = true;
	}
}

void ATetherPlayerController::SpawnHUDWidgets()
{
	// todo - better control over splitscreen/non-splitscreen widgets
	// todo - only spawn widgets after all players have joined so we actually know if the game is splitscreen
	
	// Try to determine if our game mode is splitscreen
	// If we're splitscreen only spawn widgets on the primary player
	const ATetherGameModeBase* GameModeBase = GetGameModeCDO<ATetherGameModeBase>();
	if (GameModeBase && (IsPrimaryPlayer() || GameModeBase->ShouldUseSplitscreen()))
	{
		TArray<TSubclassOf<UUserWidget>> WidgetsToSpawn = HUDWidgetClasses;
		if (GameModeBase)
		{
			WidgetsToSpawn.Append(GameModeBase->GetHUDWidgetClasses());
		}

		for (const TSubclassOf<UUserWidget>& WidgetClass : WidgetsToSpawn)
		{
			if (UUserWidget* NewWidget = CreateWidget(this, WidgetClass))
			{
				if (GameModeBase->ShouldUseSplitscreen())
				{
					// In splitscreen this widget should be added to the individual player screen 
					NewWidget->AddToPlayerScreen();
				}
				else
				{
					NewWidget->AddToViewport();
				}
			}
		}		
	}
}
