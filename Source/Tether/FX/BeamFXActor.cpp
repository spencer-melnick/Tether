// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "BeamFXActor.h"

#include "Tether/Tether.h"
#include "Tether/Gameplay/BeamComponent.h"
#include "Tether/Gameplay/BeamController.h"


ABeamFXActor::ABeamFXActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ABeamFXActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ensure(bEffectActive))
	{
		UpdateFX();
	}
}

void ABeamFXActor::GetTargets(AActor*& OutTarget1, AActor*& OutTarget2) const
{
	OutTarget1 = Target1;
	OutTarget2 = Target2;
}

FVector ABeamFXActor::GetTargetEffectLocation1() const
{
	const UBeamComponent* TargetComponent = UBeamComponent::GetComponentFromActor(Target1);
	if (ensure(TargetComponent))
	{
		return TargetComponent->GetEffectLocation();
	}

	return FVector::ZeroVector;
}

FVector ABeamFXActor::GetTargetEffectLocation2() const
{
	const UBeamComponent* TargetComponent = UBeamComponent::GetComponentFromActor(Target2);
	if (ensure(TargetComponent))
	{
		return TargetComponent->GetEffectLocation();
	}

	return FVector::ZeroVector;
}

void ABeamFXActor::GetTargetEffectLocations(FVector& OutEffectLocation1, FVector& OutEffectLocation2) const
{
	OutEffectLocation1 = GetTargetEffectLocation1();
	OutEffectLocation2 = GetTargetEffectLocation2();
}

ABeamController* ABeamFXActor::GetOwningBeamController() const
{
	return Cast<ABeamController>(GetOwner());
}

void ABeamFXActor::SetTargets(AActor* NewTarget1, AActor* NewTarget2)
{
	if (UBeamComponent::GetComponentFromActor(NewTarget1) && UBeamComponent::GetComponentFromActor(NewTarget2))
	{
		Target1 = NewTarget1;
		Target2 = NewTarget2;
	}
	else
	{
		UE_LOG(LogTetherGame, Warning, TEXT("ABeamFXActor::SetTargets() failed - %s and %s are not valid beam targets "),
			*GetNameSafe(Target1), *GetNameSafe(Target2))
	}
}

void ABeamFXActor::ClearTargets()
{
	Target1 = nullptr;
	Target2 = nullptr;
}

void ABeamFXActor::SetEffectActive(bool bNewActive)
{
	if (bNewActive != bEffectActive)
	{
		bEffectActive = bNewActive;
		SetActorTickEnabled(bEffectActive);
		SetHidden(!bEffectActive);
		BP_SetEffectActive(bEffectActive);
	}
}

void ABeamFXActor::UpdateFX()
{
	const UBeamComponent* TargetComponent1 = UBeamComponent::GetComponentFromActor(Target1);
	const UBeamComponent* TargetComponent2 = UBeamComponent::GetComponentFromActor(Target2);
	if (ensure(TargetComponent1 && TargetComponent2))
	{
		const FVector TargetLocation1 = TargetComponent1->GetEffectLocation();
		const FVector TargetLocation2 = TargetComponent2->GetEffectLocation();

		SetActorLocation((TargetLocation1 + TargetLocation2) / 2.f);
		BP_UpdateFX(TargetLocation1, TargetLocation2);
	}
}
