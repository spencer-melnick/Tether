// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "FollowerMovementComponent.generated.h"

/**
 * 
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TETHER_API UFollowerMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ScalarVelocity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RotationSpeed = 100.0f;
};
