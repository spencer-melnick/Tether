// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TetherPrimaryGameState.generated.h"


UENUM(BlueprintType)
enum class ETetherGamePhase : uint8
{
	NotStarted,
	Warmup,
	Playing,
	Ending,
	NumPhases UMETA(Hidden)
};


UCLASS()
class ATetherPrimaryGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, ETetherGamePhase, NewPhase);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGlobalHealthChanged, float, NewHealth);
	

	// Actor interface

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	// Tether functions

	/**
	 * Change the game phase, updating time and broadcasting delegates as appropriate. Will update even if the new phase
	 * is the same as the current phase.
	 */
	UFUNCTION(BlueprintCallable)
	void SetGamePhase(ETetherGamePhase NewPhase);

	UFUNCTION(BlueprintCallable)
	void SetGlobalHealth(float NewGlobalHealth);


	// Accessors

	UFUNCTION(BlueprintPure)
	ETetherGamePhase GetCurrentGamePhase() const { return GamePhase; }

	/** Returns the time in seconds when the game phase started */
	UFUNCTION(BlueprintPure)
	float GetCurrentPhaseStartTime() const { return PhaseStartTime; }

	/** Returns the time in seconds since the game phase started */
	UFUNCTION(BlueprintPure)
	float GetTimeInCurrentPhase() const;

	UFUNCTION(BlueprintPure)
	float GetGlobalHealth() const { return GlobalHealth; }
	

	// Delegates

	UPROPERTY(BlueprintAssignable)
	FOnGamePhaseChanged OnGamePhaseChanged;

	UPROPERTY(BlueprintAssignable)
	FOnGlobalHealthChanged OnGlobalHealthChanged;
	

protected:

	/** Current game phase */
	UPROPERTY(Replicated)
	ETetherGamePhase GamePhase = ETetherGamePhase::NotStarted;

	/** Game time when the current game phase started */
	UPROPERTY(Replicated)
	float PhaseStartTime = 0.f;

	/** Overall health pool of all players in the current game*/
	UPROPERTY(ReplicatedUsing = OnRep_GlobalHealth)
	float GlobalHealth = 100.f;


private:

	UFUNCTION()
	void OnRep_GlobalHealth();
};
