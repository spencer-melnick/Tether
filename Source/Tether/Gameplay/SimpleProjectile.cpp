// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "SimpleProjectile.h"

const FName ASimpleProjectile::RootComponentName(TEXT("RootComponent"));


ASimpleProjectile::ASimpleProjectile()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(RootComponentName);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ASimpleProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector NewLocation = GetActorLocation() + Velocity * DeltaSeconds;
	SetActorLocation(NewLocation, true);
}
