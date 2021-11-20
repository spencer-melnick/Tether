// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Tether/Character/TetherCharacter.h"

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

	UFUNCTION(BlueprintCallable)
	float SubtractGlobalHealth(const float DamageAmount);

	UFUNCTION(BlueprintCallable)
	float AddGlobalHealth(const float HealAmount);

	UFUNCTION(BlueprintCallable)
	void AddScore(const int Points);

	UFUNCTION(BlueprintCallable)
	int GetScore();
	
	UFUNCTION(BlueprintCallable)
	void SoftRestart();

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

	UFUNCTION(BlueprintPure)
	float GetBaseObstacleSpeed() const;

	TArray<TWeakObjectPtr<APawn>> GetCharacterPawns() const;

	UFUNCTION(BlueprintPure)
	APawn* GetClosestCharacterPawn(const FVector& Location);
	APawn* GetClosestCharacterPawn(const FVector& Location, APawn* IgnorePawn);


	// Delegates

	UPROPERTY(BlueprintAssignable)
	FOnGamePhaseChanged OnGamePhaseChanged;

	UPROPERTY(BlueprintAssignable)
	FOnGlobalHealthChanged OnGlobalHealthChanged;

	UFUNCTION(BlueprintCallable)
	void SuspendActors();

	UFUNCTION(BlueprintCallable)
	void UnsuspendActors();

	UFUNCTION(BlueprintCallable)
	bool GetIsSuspended() const { return bIsSuspended; }

private:

	void CacheActorsInitialState();
	void ReloadActors();
	
	// Replication functions

	UFUNCTION()
	void OnRep_GlobalHealth();


	UPROPERTY(VisibleInstanceOnly)
	TArray<TWeakObjectPtr<APawn>> CharacterPawns;
	
	// Replicated properties

	/** Current game phase */
	UPROPERTY(Replicated)
	ETetherGamePhase GamePhase = ETetherGamePhase::NotStarted;

	/** Game time when the current game phase started */
	UPROPERTY(Replicated)
	float PhaseStartTime = 0.f;

	/** Overall health pool of all players in the current game*/
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_GlobalHealth)
	float GlobalHealth = 100.f;

	UPROPERTY(EditDefaultsOnly)
	float MaxGlobalHealth = 100.f;

    UPROPERTY(EditAnywhere)
    int Score = 0;

	UPROPERTY()
	bool bIsSuspended = false;
};