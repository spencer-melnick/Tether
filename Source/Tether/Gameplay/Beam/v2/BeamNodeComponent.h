// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "BeamNodeComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TETHER_API UBeamNodeComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	UBeamNodeComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void PowerOn(UBeamNodeComponent* Source, UBeamNodeComponent* Origin, int Iteration = 0);

	UFUNCTION(BlueprintCallable)
	void PowerOff(int Iteration = 0);
	
	UFUNCTION(BlueprintCallable)
	bool GetPowered() const { return bPowered; }

	uint8 GetId() const { return Id; }
	
protected:
	void TryPowerNodes(int Iteration = 0);

	void ValidateConnections();
	
	bool CanReachBeam(const UBeamNodeComponent* OtherBeamComponent) const;

	TArray<UBeamNodeComponent*> GetBeamComponentsInRange(const float SearchRange) const;

private:

	void Register();

// PROPERTIES
public:
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ECollisionChannel> BeamTraceChannel;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Beam")
	float Range = 500.0f;

protected:
	UPROPERTY(Transient, VisibleInstanceOnly)
	TWeakObjectPtr<UBeamNodeComponent> PowerSource;
	
	UPROPERTY(VisibleInstanceOnly)
	bool bPowered = false;

	UPROPERTY(Transient, VisibleInstanceOnly)
	TArray<TWeakObjectPtr<UBeamNodeComponent>> NodesSupplying;

	UPROPERTY(Transient, VisibleInstanceOnly)
	TWeakObjectPtr<UBeamNodeComponent> PowerOrigin;

private:
	uint8 Id;

	uint8 MaxIterations = 10;
};