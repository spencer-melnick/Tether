// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "SimpleProjectile.generated.h"

UCLASS()
class ASimpleProjectile : public AActor
{
	GENERATED_BODY()

public:

	static const FName RootComponentName;

	ASimpleProjectile();


	// Actor overrides
	
	virtual void Tick(float DeltaSeconds) override;


	// Editor properties
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile", meta=(ClampMin="0.0"))
	float ProjectileRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	FVector Velocity;
};
