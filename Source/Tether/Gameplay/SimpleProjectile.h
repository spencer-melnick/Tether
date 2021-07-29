// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "SimpleProjectile.generated.h"

UCLASS()
class ASimpleProjectile : public AActor
{
	GENERATED_BODY()

public:

	ASimpleProjectile();

	// Actor overrides
	
	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile", meta=(ClampMin="0.0"))
	float ProjectileRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	FVector Velocity;

	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile | Launch")
	float LaunchVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile | Launch")
	float DebounceTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile | Launch")
	float Elasticity = 0.7f;


	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UPrimitiveComponent* CollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

private:

	void BouncePlayer(const FHitResult& HitResult);

	bool Sweep(const FVector& Distance, FHitResult& OutHit) const;
	
	FVector InitialScale;

	FTimerHandle DebounceTimerHandle;

	bool bDebounce = false;
};
