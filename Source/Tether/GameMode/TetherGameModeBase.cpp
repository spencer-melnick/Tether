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
	if (ATetherPlayerController* TetherPlayerController = Cast<ATetherPlayerController>(Player))
	{
		const int SpawnedPlayerSlot = TetherPlayerController->GetPlayerSlot();
		if (ATetherWorldSettings* WorldSettings = Cast<ATetherWorldSettings>(GetWorld()->GetWorldSettings()))
		{
			if (APlayerStart* PlayerStart = WorldSettings->GetPlayerStart(SpawnedPlayerSlot))
			{
				UE_LOG(LogTetherGame, Verbose, TEXT("Found player start for player at slot %i."), SpawnedPlayerSlot);
				return PlayerStart;
			}
		}
		UE_LOG(LogTetherGame, Warning, TEXT("Unable to find spawn for player in slot %i. Check that the PlayerSpawn is set in World Settings."), SpawnedPlayerSlot);
	}
	else
	{
		UE_LOG(LogTetherGame, Warning, TEXT("Could not cast the controller to ATetherPlayerController."));
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}


FString ATetherGameModeBase::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
	const FString& Options, const FString& Portal)
{
	if (ATetherPlayerController* TetherPlayerController = Cast<ATetherPlayerController>(NewPlayerController))
	{
		const int PlayerSlotNumber = GetNumPlayers() - 1;
		TetherPlayerController->SetPlayerSlot(PlayerSlotNumber);
		UE_LOG(LogTetherGame, Verbose, TEXT("Setting player slot number to %i."), PlayerSlotNumber);
	}
	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
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