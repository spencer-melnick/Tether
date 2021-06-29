// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Tether/GameMode/TetherGameModeBase.h"

void ATetherPlayerController::SpawnHUDWidgets()
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	const ATetherGameModeBase* GameMode = GameState ? GameState->GetDefaultGameMode<ATetherGameModeBase>() : nullptr;
	if (!bSpawnedHUDWidgets && GameMode)
	{
		bSpawnedHUDWidgets = true;
		const bool bNeedsAnyWidgets = IsPrimaryPlayer() || GameMode->ShouldUseSplitscreen();
		
		if (bNeedsAnyWidgets)
		{
			if (IsPrimaryPlayer())
			{
				// Spawn whole screen widgets
				for (const TSubclassOf<UUserWidget>& WidgetClass : GameMode->GetGameWidgetClasses())
				{
					if (UUserWidget* NewWidget = CreateWidget(this, WidgetClass))
					{
						NewWidget->AddToViewport();
					}
				}
			}
			
			TArray<TSubclassOf<UUserWidget>> CombinedPlayerWidgetClasses = PlayerWidgetClasses;
			CombinedPlayerWidgetClasses.Append(GameMode->GetPlayerWidgetClasses());

			// Spawn player unique widgets
			for (const TSubclassOf<UUserWidget>& WidgetClass : CombinedPlayerWidgetClasses)
			{
				if (UUserWidget* NewWidget = CreateWidget(this, WidgetClass))
				{
					NewWidget->AddToPlayerScreen();
				}
			}
		}
	}
}
