// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NiagaraComponent.h"
#include "TetherGameModeBase.generated.h"


class AEdisonActor;
class ATetherCharacter;

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
	TSubclassOf<AEdisonActor> TetherEffectClass;

private:

	// Tether functions

	void SpawnEdisons();
	void CheckAllTethers();
	bool AreCharactersTethered(const ATetherCharacter* FirstCharacter, const ATetherCharacter* SecondCharacter) const;


	// Tether effects

	UPROPERTY(Transient)
	TArray<AEdisonActor*> Edisons;

	TMap<const APlayerController*, TMap<const APlayerController*, AEdisonActor*>> EdisonMap;
};
