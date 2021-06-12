// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "EdisonActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

UCLASS(Blueprintable)
class AEdisonActor : public AActor
{
	GENERATED_BODY()

public:

	static const FName NigaraComponentName;
	

	AEdisonActor();
	

	UFUNCTION(BlueprintImplementableEvent)
	void SetTetherVisibility(bool bNewVisibility);

	UFUNCTION(BlueprintImplementableEvent)
	void SetEndpoints(const FVector StartLocation, const FVector EndLocation);

	UFUNCTION(BlueprintCallable)
	UNiagaraComponent* GetNiagaraComponent() const { return NiagaraComponent; }

private:

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	UNiagaraComponent* NiagaraComponent;
	
};
