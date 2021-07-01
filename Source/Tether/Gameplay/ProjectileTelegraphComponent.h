// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "SimpleProjectile.h"
#include "Components/SceneComponent.h"
#include "ProjectileTelegraphComponent.generated.h"


class UDecalComponent;

UCLASS( ClassGroup=(Tether), meta=(BlueprintSpawnableComponent) )
class TETHER_API UProjectileTelegraphComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	static const FName DecalComponentName;
	
	UProjectileTelegraphComponent();

	virtual void BeginPlay() override;


	// Telegraph controls
	
	UFUNCTION(BlueprintCallable)
	void SpawnTelegraphForActor(TSubclassOf<ASimpleProjectile> ActorClass);

	UFUNCTION(BlueprintCallable)
	void Display(float Duration);


	// Editor properties

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decal")
	UMaterialInterface* TelegraphDecal;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decal")
	float Length;

private:

	UPROPERTY()
	UDecalComponent* DecalComponent;

	FTimerHandle VisibilityTimer;
	bool bVisible;
	
};
