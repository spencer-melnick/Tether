// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "BeamComponent.h"
#include "BeamController.h"
#include "Tether/Tether.h"

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

UBeamComponent* UBeamComponent::GetComponentFromActor(const AActor* Actor)
{
	if (Actor && Actor->Implements<UBeamTarget>())
	{
		return IBeamTarget::Execute_GetBeamComponent(Actor);
	}

	return nullptr;
}

FVector UBeamComponent::GetEffectLocation() const
{
	const USceneComponent* BeamParent = GetAttachParent();

	if (!EffectLocationSocket.IsNone())
	{
		if (BeamParent && BeamParent->DoesSocketExist(EffectLocationSocket))
		{
			return BeamParent->GetSocketLocation(EffectLocationSocket);
		}

#if !UE_BUILD_SHIPPING
		UE_LOG(LogTetherGame, Warning,
			TEXT("BeamComponent::GetEffectLocation() failed on %s ")
			TEXT("Attachment parent %s on actor %s does not have socket %s"),
			*GetNameSafe(this), *GetNameSafe(BeamParent), *GetNameSafe(GetAttachmentRootActor()),
			*EffectLocationSocket.ToString());
#endif
	}

	return GetComponentLocation();
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

TArray<FName> UBeamComponent::GetAttachmentSockets() const
{
	if (const USceneComponent* SceneParent = GetAttachParent())
	{
		return SceneParent->GetAllSocketNames();
	}

	return TArray<FName>();
}
