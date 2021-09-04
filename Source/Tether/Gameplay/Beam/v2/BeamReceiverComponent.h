// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "BeamNodeComponent.h"
#include "Components/ActorComponent.h"
#include "BeamReceiverComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TETHER_API UBeamReceiverComponent : public UBeamNodeComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UBeamReceiverComponent();

	virtual void PowerOn(UBeamNodeComponent* Source, UBeamNodeComponent* Origin, int Iteration) override;

	virtual void PowerOff(int Iteration) override;
	
	void RecieveEnergy(const float EnergyRatio);

	UFUNCTION(BlueprintCallable)
	float GetPowerPercentage() const;
	
	UPROPERTY(EditAnywhere, Category = "Beam|Receiver")
	float EnergyConsumptionRate = 10.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Beam|Receiver")
	bool bActive = false;

private:
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Beam|Receiver")
	float PowerPercentage = 0.0f;
};
