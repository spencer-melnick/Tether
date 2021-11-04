// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Conveyor.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TETHER_API UConveyorComponent : public UBoxComponent
{
	GENERATED_BODY()
	
public:
	UConveyorComponent();

	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void BeginPlay() override;

	FVector GetAppliedVelocity() const;


private:
	UPROPERTY(EditAnywhere)
	FVector BeltVelocity;
};
