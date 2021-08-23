// Copyright 2021 Spencer Melnick, Stephen Melnick

#include "TetherGameModeBase.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Tether/Controller/TetherPlayerController.h"
#include "Tether/Core/TetherDeveloperSettings.h"
#include "Tether/Core/TetherGameInstance.h"
#include "Tether/Tether.h"
#include "Tether/Core/TetherWorldSettings.h"

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
		// GEngine->GameViewport->SetForceDisableSplitscreen(!bUseSplitscreen);
	}

#if WITH_EDITOR
	const UTetherGameInstance* GameInstance = GetGameInstance<UTetherGameInstance>();
	if (IsPlayingInEditor() && ShouldSpawnPIEPlayers() && GameInstance && GameInstance->InStartingPIEWorld())
	{
		UE_LOG(LogTetherGame, Display, TEXT("TetherGameModeBase - spawning PIE players"));
		SpawnPIEPlayers();
	}
#endif

	SpawnHUDWidgets();
}


AActor* ATetherGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	const int SpawnedPlayerIndex = GetNumPlayers() - 1;
	if (ATetherWorldSettings* WorldSettings = Cast<ATetherWorldSettings>(GetWorld()->GetWorldSettings()))
	{
		if (APlayerStart* PlayerStart = WorldSettings->GetPlayerStart(SpawnedPlayerIndex))
		{
			return PlayerStart;
		}
	}
	
	UE_LOG(LogTetherGame, Warning, TEXT("Could not find a spawn for player number %i."), SpawnedPlayerIndex);

	return Super::ChoosePlayerStart_Implementation(Player);
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
		GameInstance->SetNumberOfPlayers(NumPIEPlayers);
	}	
}
#endif