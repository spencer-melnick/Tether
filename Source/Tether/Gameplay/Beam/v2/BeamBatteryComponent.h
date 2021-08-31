// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "BeamNodeComponent.h"
#include "Components/ActorComponent.h"
#include "BeamBatteryComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TETHER_API UBeamBatteryComponent : public UBeamNodeComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UBeamBatteryComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float ConsumeEnergy(const float EnergyConsumed);
	
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
};
