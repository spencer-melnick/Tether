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

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Telegraph")
	UProjectileEmitterComponent* ProjectileEmitterComponent;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Telegraph")
	UDecalComponent* TelegraphComponent;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Emitter")
	const TSubclassOf<ASimpleProjectile> ProjectileClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Emitter")
	float Velocity;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Emitter")
	float Distance;

	float ProjectileLifetime;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Telegraph")
	bool bTelegraph;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Telegraph")
	float WarningTime;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Telegraph")
	UMaterialInterface* TelegraphDecal;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Telegraph")
	UMaterialInterface* ArrowMaterial;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Telegraph")
	USkeletalMesh* ArrowMesh;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Telegraph")
	UAnimationAsset* FrontAnimation;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Telegraph")
	UAnimationAsset* MiddleAnimation;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Telegraph")
	UAnimationAsset* BackAnimation;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	void DisplayTelegraph();
	
	void HideTelegraph();

	void SetTelegraphSize();
	
	FTimerHandle WarningHandle;

	UPROPERTY(VisibleAnywhere)
	TArray<USkeletalMeshComponent*> ArrowComponents;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Emitter Events")
	void DisplayWarning();

};
