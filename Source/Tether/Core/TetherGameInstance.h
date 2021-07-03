// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "TetherGameInstance.generated.h"

UCLASS()
class UTetherGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void SetNumberOfPlayers(int32 NumPlayers);


	// Game instance interface

	virtual void OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld) override;
	
#if WITH_EDITOR

	// Accessors

	/** Returns true if the current world is the first world that was loaded in a PIE session */
	bool InStartingPIEWorld() const { return bInStartingPIEWorld; }


private:

	bool bInStartingPIEWorld = false;
	
#endif
	
};
