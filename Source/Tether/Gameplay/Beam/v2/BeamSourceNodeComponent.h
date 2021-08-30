// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "BeamNodeComponent.h"
#include "BeamSourceNodeComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TETHER_API UBeamSourceNodeComponent : public UBeamNodeComponent
{
	GENERATED_BODY()

public:	
	UBeamSourceNodeComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power")
	float StoredEnergy = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power")
	bool bLimitedCharge = false;
};
