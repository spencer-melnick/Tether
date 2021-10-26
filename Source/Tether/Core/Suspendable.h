// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "Suspendable.generated.h"

/**
 * 
 */
UINTERFACE(Blueprintable)
class USuspendable : public UInterface
{
	GENERATED_BODY()
};

class ISuspendable
{
	GENERATED_BODY()

public:

	virtual void Suspend() = 0;
	virtual void Unsuspend() = 0;

	virtual void CacheInitialState() = 0;
	virtual void Reload() = 0;

};
