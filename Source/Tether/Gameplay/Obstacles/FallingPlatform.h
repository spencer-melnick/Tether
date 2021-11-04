// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FallingPlatform.generated.h"

UCLASS()
class TETHER_API AFallingPlatform : public AActor
{
	GENERATED_BODY()
	
public:	
	AFallingPlatform();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

private:
	UPROPERTY()
	TArray<UStaticMeshComponent*> UpdatedComponents;
	
private:
	void BeginFalling();
};
