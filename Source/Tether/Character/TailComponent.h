// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Containers/Queue.h"

#include "TailComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TETHER_API UTailComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTailComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	FVector StoreLocation(const FVector& Location);

public:	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Motion Tracking")
	int QueueSize = 20;
	
private:
	TQueue<FVector> PreviousLocations;

	FVector Offset;
	
	int QueueCount = 0;
		
};
