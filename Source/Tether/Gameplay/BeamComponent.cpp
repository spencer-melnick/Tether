// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "BeamComponent.h"
#include "BeamController.h"

UBeamComponent::UBeamComponent()
{
	
}

void UBeamComponent::BeginPlay()
{
	Super::BeginPlay();

	SetMode(static_cast<EBeamComponentMode>(DefaultMode));
}

void UBeamComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BeamController)
	{
		BeamController->HandleTargetDestroyed(GetOwner());
	}
}

void UBeamComponent::SetMode(EBeamComponentMode NewMode)
{
	if (Mode != NewMode)
	{
		const EBeamComponentMode OldMode = Mode;
		Mode = NewMode;

		if (BeamController)
		{
			BeamController->HandleTargetModeChanged(GetOwner(), OldMode, NewMode);
		}
	}
}

void UBeamComponent::SetStatus(EBeamComponentStatus NewStatus)
{
	if (Status != NewStatus)
	{
		const EBeamComponentStatus OldStatus = Status;
		Status = NewStatus;

		OnStatusChanged.Broadcast(OldStatus, NewStatus);
	}
}
