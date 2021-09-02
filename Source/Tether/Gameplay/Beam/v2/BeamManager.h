// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "BeamNodeComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Info.h"

#include "BeamManager.generated.h"

UCLASS()
class TETHER_API ABeamManager : public AInfo
{
	GENERATED_BODY()

public:
	ABeamManager();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	TArray<TWeakObjectPtr<UBeamNodeComponent>>& GetNodes() { return Nodes; }

	UFUNCTION(BlueprintCallable)
	void AddNode(UBeamNodeComponent* Node);

	UFUNCTION(BlueprintCallable)
	uint8 AddUniqueNode(UBeamNodeComponent* Node);

	void Cleanup();
	
	float GetTickInterval() const;
	
private:

	UPROPERTY(VisibleInstanceOnly)
	TArray<TWeakObjectPtr<UBeamNodeComponent>> Nodes;

	UPROPERTY(VisibleInstanceOnly)
	float TickInterval = 0.0f;
};
