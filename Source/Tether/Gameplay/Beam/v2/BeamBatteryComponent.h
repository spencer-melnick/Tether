// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "BeamNodeComponent.h"
#include "BeamReceiverComponent.h"

#include "BeamBatteryComponent.generated.h"

USTRUCT() struct FEnergyRequest
{
	GENERATED_BODY()

	FEnergyRequest();
	FEnergyRequest(float RequestAmount, UBeamReceiverComponent* Requester);
	
	UPROPERTY(VisibleInstanceOnly)
	float RequestAmount;

	UPROPERTY(VisibleInstanceOnly)
	TWeakObjectPtr<UBeamReceiverComponent> Requester;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TETHER_API UBeamBatteryComponent : public UBeamNodeComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UBeamBatteryComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void ReceiveEnergyRequest(const float RequestAmount, UBeamReceiverComponent* Requester);

	void PowerReceivers(const float DeltaTime);

	
private:
	UFUNCTION(BlueprintCallable)
	float GetStoragePercent() const;
	
public:
	UPROPERTY(EditAnywhere, Category = "Beam|Battery")
	float MaxEnergy = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Beam|Battery")
	float Energy = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Beam|Battery")
	float GenerationRate = 0.0f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Beam|Battery|Supply")
	TArray<FEnergyRequest> Requests;

	UPROPERTY(VisibleAnywhere, Category = "Beam|Battery|Supply")
	float RequestedEnergy = 0.0f;
};
