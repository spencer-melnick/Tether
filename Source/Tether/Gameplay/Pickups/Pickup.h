// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "Tether/Core/Suspendable.h"

#include "Pickup.generated.h"

UCLASS()
class TETHER_API APickup : public AActor, public ISuspendable
{
	GENERATED_BODY()

public:
	APickup();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnPickedUp();

	
	virtual void Suspend() override;
	virtual void Unsuspend() override;
	virtual void CacheInitialState() override;
	virtual void Reload() override;

	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pickup")
	float MaxAttractRadius = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pickup")
	float MaxAttractVelocity = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pickup")
	float MinAttractVelocity = 50.0f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Pickup")
	float WarmupTime = 0.5f;
	
	UPROPERTY(VisibleAnywhere)
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pickup")
	int PointValue = 100;

	
private:
	UPROPERTY(EditDefaultsOnly)
	USphereComponent* PickupSphere;

	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleInstanceOnly)
	bool bIsPickedUp = true;

	FTimerHandle WarmupTimerHandle;
};
