// Copyright (c) 2021 Spencer Melnick

#include "EdisonActor.h"

#include "NiagaraComponent.h"


const FName AEdisonActor::NigaraComponentName(TEXT("NiagaraComponent"));


AEdisonActor::AEdisonActor()
{
	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(NigaraComponentName, true);
	NiagaraComponent->SetupAttachment(RootComponent);
}

