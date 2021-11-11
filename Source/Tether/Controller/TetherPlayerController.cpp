// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Tether/Tether.h"
#include "Tether/GameMode/TetherGameModeBase.h"

void ATetherPlayerController::SpawnHUDWidgets()
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	const ATetherGameModeBase* GameMode = GameState ? GameState->GetDefaultGameMode<ATetherGameModeBase>() : nullptr;
	if (!bSpawnedHUDWidgets && GameMode)
	{
		UE_LOG(LogTetherGame, Verbose, TEXT("HUD widgets not spawned, initializing"));
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

			if (UUserWidget* NewWidget = CreateWidget(this, FadeWidgetClass))
			{
				FadeWidget = NewWidget;
				NewWidget->AddToPlayerScreen();
			}
			
			UE_LOG(LogTetherGame, Verbose, TEXT("Spawning %i different player HUD widgets."), CombinedPlayerWidgetClasses.Num());
			
			// Spawn player unique widgets
			for (const TSubclassOf<UUserWidget>& WidgetClass : CombinedPlayerWidgetClasses)
			{
				if (UUserWidget* NewWidget = CreateWidget(this, WidgetClass))
				{
					UE_LOG(LogTetherGame, Verbose, TEXT("Spawning individual player widget %s"), *WidgetClass->GetName());
					NewWidget->AddToPlayerScreen();
				}
			}
		}
	}
}


void ATetherPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (GetPawn() && GetPawn()->HasActiveCameraComponent())
	{
		return;
	}
	
	if (UWorld* World = GetWorld())
	{
		for (AActor* Actor : World->GetCurrentLevel()->Actors)
		{
			if (Actor && Actor->ActorHasTag(TEXT("Camera")))
			{
				SetViewTarget(Actor);
				return;
			}
		}
	}
}


void ATetherPlayerController::SetPlayerSlot(const int SlotNumber)
{
	PlayerSlotNumber = SlotNumber;
}


int ATetherPlayerController::GetPlayerSlot() const
{
	return PlayerSlotNumber;
}
