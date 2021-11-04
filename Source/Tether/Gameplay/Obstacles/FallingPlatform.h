// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tether/Core/Suspendable.h"

#include "FallingPlatform.generated.h"

UCLASS()
class TETHER_API AFallingPlatform : public AActor, public ISuspendable
{
	GENERATED_BODY()
	
public:	
	AFallingPlatform();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void Suspend() override;
	virtual void Unsuspend() override;
	virtual void CacheInitialState() override;
	virtual void Reload() override;

	UFUNCTION(BlueprintImplementableEvent)
	void StartShaking();

	UFUNCTION(BlueprintCallable)
	bool GetIsShaking() const { return bIsShaking; }
		
private:
	void Destabilize();
	
	void BeginFalling();

	void Despawn();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Falling")
	FVector Velocity;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Falling|Timing")
	float FallTime = 5.0f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Falling")
	bool bRespawn = true;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Falling|Timing")
	float RespawnTime = 5.0f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Falling|Timing")
	float ShakeTime = 5.0f;
	
private:
	UPROPERTY()
	TArray<UPrimitiveComponent*> UpdatedComponents;

	UPROPERTY()
	bool bIsFalling = false;

	UPROPERTY()
	bool bIsShaking = false;

	bool bIsSuspended = false;
	
	FTimerHandle FallTimerHandle;

	FVector InitialLocation;
	FRotator InitialRotation;
};
