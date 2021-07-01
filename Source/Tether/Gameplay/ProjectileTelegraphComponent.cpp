// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "ProjectileTelegraphComponent.h"

#include "Components/DecalComponent.h"


const FName UProjectileTelegraphComponent::DecalComponentName(TEXT("DecalComponent"));

UProjectileTelegraphComponent::UProjectileTelegraphComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bVisible = false;
}

void UProjectileTelegraphComponent::BeginPlay()
{
	Super::BeginPlay();
}


void UProjectileTelegraphComponent::SpawnTelegraphForActor(TSubclassOf<ASimpleProjectile> ActorClass)
{
	const ASimpleProjectile* ActorCDO = ActorClass ? ActorClass->GetDefaultObject<ASimpleProjectile>() : nullptr;
	if (ActorCDO)
	{
		DecalComponent = NewObject<UDecalComponent>(this);
		DecalComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
		DecalComponent->RegisterComponent();

		DecalComponent->SetWorldScale3D(FVector(0.5));

		// DecalComponent->SetFadeOut(0, 0, false);
		DecalComponent->SetDecalMaterial(TelegraphDecal);
		const float Size = ActorCDO->ProjectileRadius * 2;
		DecalComponent->DecalSize = FVector(Size, Length, Size);
		DecalComponent->SetRelativeLocation(FVector(Length / 2.f, 0, 0));
		DecalComponent->SetRelativeRotation(FRotator(90, 90, 0));
		DecalComponent->SetVisibility(false);
	}
}

void UProjectileTelegraphComponent::Display(float Duration)
{
	if (UWorld* World = GetWorld())
	{
		DecalComponent->SetVisibility(true);
		World->GetTimerManager().SetTimer(VisibilityTimer, FTimerDelegate::CreateWeakLambda(this, [this]
		{
			DecalComponent->SetVisibility(false);
		}), Duration, false);
	}
}


