// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TetherCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class TETHER_API UTetherCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	
	UFUNCTION(BlueprintCallable)
	void SetAnchorRotation(const FRotator Rotation);

	UFUNCTION(BlueprintCallable)
	void SetAnchorLocation(const FVector Location);

private:
	FRotator GetAnchorDeltaRotation(float DeltaTime);
	FRotator AnchorRotation;

	FVector GetAnchorDeltaLocation(float DeltaTime);
	FVector AnchorLocation;
	
};

UENUM(BlueprintType)
enum class ETetherMovementType : uint8
{
	Anchored UMETA(DisplayName="Anchored"),
	Deflected UMETA(DisplayName="Deflected")
};