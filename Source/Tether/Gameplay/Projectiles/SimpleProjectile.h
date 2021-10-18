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

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FProjectileBounceEvent);
	FProjectileBounceEvent& OnProjectileBounce() { return ProjectileBounceEvent; }
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = 0.0f))
	float ProjectileRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FVector Velocity;

	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Launch")
	float LaunchVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Launch", meta = (ClampMin = 0.0f))
	float DebounceTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Launch", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float Elasticity = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Launch", meta = (ClampMin = 0.0f))
	float DeflectTime = 0.2f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UShapeComponent* CollisionComponent;

private:

	void BouncePlayer(const FHitResult& HitResult);

	bool Sweep(const FVector& Distance, FHitResult& OutHit) const;
	
	FVector InitialScale;

	FTimerHandle DebounceTimerHandle;

	bool bDebounce = false;

	UPROPERTY(BlueprintAssignable)
	FProjectileBounceEvent ProjectileBounceEvent;
};
