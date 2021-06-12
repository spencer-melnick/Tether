// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NiagaraComponent.h"
#include "TetherGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class TETHER_API ATetherGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	ATetherGameModeBase();


	// Actor overrides

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;


	// Editor properties

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tether")
	float MaxUntetheredTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tether")
	TEnumAsByte<ECollisionChannel> TetherTraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tether")
	UNiagaraSystem* TetherEffectSystem;

private:

	// Tether functions

	void CheckAllTethers();
	bool ArePlayersTethered(const APlayerController* FirstPlayer, const APlayerController* SecondPlayer) const;


	// Tether effects

	UPROPERTY(Transient)
	UNiagaraComponent* TetherEffectComponent;
	
};
