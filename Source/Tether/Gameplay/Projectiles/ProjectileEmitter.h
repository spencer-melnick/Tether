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
	AProjectileEmitter();

	virtual void Tick(float DeltaTime) override;
	
    virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

	
	UFUNCTION(BlueprintCallable)
	void FireProjectile();

	UFUNCTION(BlueprintImplementableEvent, Category = "Emitter Events")
	void DisplayWarning(const float Alpha);

private:
	void DisplayFloorMarker();
	
	void HideFloorMarker();

	void RecalculateFloorMarkerSize();

	void SetMaterials();

	
public:
	UPROPERTY(VisibleAnywhere)
	UProjectileEmitterComponent* ProjectileEmitterComponent;

	UPROPERTY(VisibleAnywhere)
	UDecalComponent* FloorMarkerComponent;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Emitter|Queue")
	int MaxQueuedProjectiles = 10;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Emitter|Queue")
	int QueuedProjectiles = 0;

	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Emitter|Timing")
	bool bTelegraph;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Emitter|Timing")
	float WarningTime;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Emitter|Timing")
	UCurveFloat* AlphaCurve;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Emitter|Floor Marker|Prediction")
	UMaterialInterface* FloorMarkerDecalMaterial;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Emitter|Floor Marker|Arrows")
	UMaterialInterface* ArrowMaterial;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Emitter|Floor Marker|Arrows")
	USkeletalMesh* ArrowMesh;

	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Emitter|Arrows|Animation")
	UAnimationAsset* FrontAnimation;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Emitter|Arrows|Animation")
	UAnimationAsset* MiddleAnimation;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Emitter|Arrows|Animation")
	UAnimationAsset* BackAnimation;
	

private:	
	FTimerHandle WarningHandle;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Emitter|Arrows")
	TArray<USkeletalMeshComponent*> ArrowComponents;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* FloorMarkerMaterialDynamic;
	
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* ArrowMaterialDynamic;

	float AlphaFallback = 0.0f;
};
