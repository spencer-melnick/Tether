// Copyright 2021 Spencer Melnick, Stephen Melnick

#include "TetherGameModeBase.h"

#include "Tether/Controller/TetherPlayerController.h"
#include "Tether/Core/TetherDeveloperSettings.h"
#include "Tether/Core/TetherGameInstance.h"
#include "Tether/Tether.h"

ATetherGameModeBase::ATetherGameModeBase()
{
	PlayerControllerClass = ATetherPlayerController::StaticClass();
}

void ATetherGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// Disable splitscreen if this mode should not have splitscreen enabled
	if (ensure(GEngine && GEngine->GameViewport))
	{
		GEngine->GameViewport->SetForceDisableSplitscreen(!bUseSplitscreen);
	}

#if WITH_EDITOR
	if (IsPlayingInEditor())
	{
		SpawnPIEPlayers();
	}
#endif

	SpawnHUDWidgets();
}

const ATetherGameModeBase* ATetherGameModeBase::GetGameModeCDO(const UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		const UWorld* World = WorldContextObject->GetWorld();
		const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
		const TSubclassOf<AGameModeBase>& GameModeClass = GameState ? GameState->GameModeClass : nullptr;
		if (GameModeClass->IsChildOf<ATetherGameModeBase>())
		{
			return GameModeClass->GetDefaultObject<ATetherGameModeBase>();
		}
	}

	return nullptr;
}

bool ATetherGameModeBase::IsPlayingInEditor() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->IsPlayInEditor();
	}
	return false;
}

void ATetherGameModeBase::SpawnHUDWidgets()
{
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			if (ATetherPlayerController* PlayerController = Cast<ATetherPlayerController>(Iterator->Get()))
			{
				PlayerController->SpawnHUDWidgets();
			}
		}
	}
}

#if WITH_EDITOR
void ATetherGameModeBase::SpawnPIEPlayers()
{
	const UTetherDeveloperSettings* DeveloperSettings = GetDefault<UTetherDeveloperSettings>();
	const UWorld* World = GetWorld();
	UTetherGameInstance* GameInstance = World ? World->GetGameInstance<UTetherGameInstance>() : nullptr;
	
	if (DeveloperSettings && GameInstance)
	{
		const int32 NumPIEPlayers = FMath::Min(DeveloperSettings->NumPIEPlayers, 4);
		while (GameInstance->GetNumLocalPlayers() < NumPIEPlayers)
		{
			FString OutError;
			if (!GameInstance->CreateLocalPlayer(-1, OutError, true))
			{
				UE_LOG(LogTetherGame, Error, TEXT("TetherGameModeBase::SpawnPIEPlayers failed: %s"), *OutError);
				break;
			}
		}
	}	
}
#endif
