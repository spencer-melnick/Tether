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
	// Sets default values for this component's properties
	ULinearMovementComponent();

	UFUNCTION(BlueprintCallable)
	void SetVelocity(const FVector& Velocity);
	void SetVelocity(const float& Velocity);


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Movement")
	FRotator Direction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Movement")
	FVector LinearVelocity;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

};
