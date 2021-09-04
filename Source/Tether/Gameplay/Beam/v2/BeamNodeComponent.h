// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "NiagaraSystem.h"
#include "Components/SceneComponent.h"
#include "BeamNodeComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TETHER_API UBeamNodeComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	UBeamNodeComponent();

	virtual void BeginPlay() override final;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintCallable)
	virtual void PowerOn(UBeamNodeComponent* Source, UBeamNodeComponent* Origin, int Iteration = 0);

	UFUNCTION(BlueprintCallable)
	virtual void PowerOff(int Iteration = 0);

	void FindNewNodes(int Iteration = 0);

	void ValidateConnections();
	
	UFUNCTION(BlueprintCallable)
	bool GetPowered() const { return bPowered || bSelfPowered; }

	TWeakObjectPtr<UBeamNodeComponent> GetOrigin() const { return PowerOrigin; }
	
	uint8 GetId() const { return Id; }

	
protected:
	bool CanReachBeam(const UBeamNodeComponent* OtherBeamComponent) const;

	TArray<UBeamNodeComponent*> GetBeamComponentsInRange(const float SearchRange) const;

	
private:
	void Register();

	void SpawnEffectComponent(UBeamNodeComponent* OtherNode);

// PROPERTIES
public:
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ECollisionChannel> BeamTraceChannel;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Beam")
	float Range = 500.0f;

protected:
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Beam")
	TWeakObjectPtr<UBeamNodeComponent> PowerSource;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Beam")
	TWeakObjectPtr<UBeamNodeComponent> PowerOrigin;

	UPROPERTY(EditDefaultsOnly, Category = "Beam|FX")
	UNiagaraSystem* BeamEffect;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Beam")
	bool bPowered = false;

	bool bActiveWhenUnpowered = false;

	bool bRecieveConnections = true;
	
	bool bSendConnections = true;

	bool bSelfPowered = false;
	
private:
	UPROPERTY(VisibleInstanceOnly)
	uint8 Id;

	uint8 MaxIterations = 10;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Beam")
	TArray<TWeakObjectPtr<UBeamNodeComponent>> NodesSupplying;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Beam|FX")
	TArray<UNiagaraComponent*> BeamEffects; 

};

USTRUCT()
struct FBeamConnection
{
	GENERATED_BODY()
	
public:
	FBeamConnection();
	FBeamConnection(UBeamNodeComponent* Child, UNiagaraComponent* Effect);

	UPROPERTY(Transient)
	UBeamNodeComponent* Child;

	UPROPERTY(Transient)
	UNiagaraComponent* Effect;	
};