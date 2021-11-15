// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "NiagaraComponent.h"
#include "GameFramework/Actor.h"
#include "Laserbeam.generated.h"

UCLASS()
class TETHER_API ALaserbeam : public AActor
{
	GENERATED_BODY()
	
public:	
	ALaserbeam();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;

	
private:
	void TraceLaser(const FVector Location, const FVector DirectionVector, const float MaxDistance, const float DeltaTime);

	void ResizeLaserMesh();

public:
	UPROPERTY(EditDefaultsOnly)
	UNiagaraComponent* SmokeEffect;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* LaserMeshComponent;

	UPROPERTY(VisibleAnywhere)
	UArrowComponent* ArrowComponent;

	UPROPERTY(EditDefaultsOnly)
	USceneComponent* LaserSource;


	UPROPERTY(EditDefaultsOnly)
	FVector Velocity;

	UPROPERTY(EditDefaultsOnly)
	float Lifetime = 10.0f;
	
	UPROPERTY(EditDefaultsOnly)
	FCollisionProfileName CollisionProfileName;

	UPROPERTY(EditAnywhere)
	float MaxLength = 250.0f;
	
	UPROPERTY(EditAnywhere)
	float LaserDamage = 20.0f;

private:
	UPROPERTY(VisibleInstanceOnly)
	float LaserLength;

	float LaserMeshLength;
};
