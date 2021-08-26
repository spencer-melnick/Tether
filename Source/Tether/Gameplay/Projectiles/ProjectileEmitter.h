// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "Components/DecalComponent.h"
#include "ProjectileEmitterComponent.h"
#include "GameFramework/Actor.h"
#include "ProjectileEmitter.generated.h"

UCLASS()
class TETHER_API AProjectileEmitter : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectileEmitter();

	UFUNCTION(BlueprintCallable)
	void FireProjectile();

	virtual void OnConstruction(const FTransform& Transform) override;	

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Components")
	UProjectileEmitterComponent* ProjectileEmitterComponent;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Components")
	UDecalComponent* FloorMarkerComponent;
	
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Emitter")
	const TSubclassOf<ASimpleProjectile> ProjectileClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Emitter")
	float Velocity;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Emitter")
	float Distance;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Emitter|Queue")
	int MaxQueuedProjectiles = 10;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Emitter|Queue")
	int QueuedProjectiles = 0;

	float ProjectileLifetime;

	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Floor Marker|Timing")
	bool bTelegraph;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Floor Marker|Timing")
	float WarningTime;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Floor Marker|Prediction")
	UMaterialInterface* FloorMarkerDecalMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Floor Marker|Prediction")
	UCurveFloat* AlphaCurve;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Floor Marker|Arrows")
	UMaterialInterface* ArrowMaterial;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Floor Marker|Arrows")
	USkeletalMesh* ArrowMesh;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Floor Marker|Arrows|Animation")
	UAnimationAsset* FrontAnimation;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Floor Marker|Arrows|Animation")
	UAnimationAsset* MiddleAnimation;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Floor Marker|Arrows|Animation")
	UAnimationAsset* BackAnimation;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	void DisplayFloorMarker();
	
	void HideFloorMarker();

	void RecalculateFloorMarkerSize();

	void SetArrowMaterial(UMaterialInstanceDynamic* Material);

	void SetFloorMarkerMaterial(UMaterialInstanceDynamic* Material);
	
	FTimerHandle WarningHandle;

	UPROPERTY()
	TArray<USkeletalMeshComponent*> ArrowComponents;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* FloorMarkerMaterialDynamic;
	
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* ArrowMaterialDynamic;

	float AlphaFallback = 0.0f;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Emitter Events")
	void DisplayWarning(const float Alpha);

};
