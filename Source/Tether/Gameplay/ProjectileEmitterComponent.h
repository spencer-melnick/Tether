// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "SimpleProjectile.h"
#include "Components/SceneComponent.h"

#include "ProjectileEmitterComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TETHER_API UProjectileEmitterComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UProjectileEmitterComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void FireProjectile();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	TSubclassOf<ASimpleProjectile> ProjectileType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	float ProjectileVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	float ProjectileLifetime = 1.0f;
};
