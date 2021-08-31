// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

#include "BeamNodeComponent.h"
#include "BeamSourceNodeComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TETHER_API UBeamSourceNodeComponent : public UBeamNodeComponent
{
	GENERATED_BODY()

public:	
	UBeamSourceNodeComponent();

};
