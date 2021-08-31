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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category = "Beam|Receiver")
	float EnergyConsumptionRate = 10.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Beam|Reciever")
	bool bActive = false;
};
