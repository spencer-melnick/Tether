// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LinearMovementComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TETHER_API ULinearMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULinearMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void SetVelocity(const FVector& InVelocity);
	void SetVelocity(const float& InVelocity);
	
protected:
	virtual void BeginPlay() override;

private:
	void Move(const float DeltaTime);

public:	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Movement")
	FRotator Direction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Movement")
	FVector Velocity;
};
