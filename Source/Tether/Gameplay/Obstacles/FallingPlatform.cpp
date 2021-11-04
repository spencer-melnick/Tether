// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "FallingPlatform.h"

#include "GameFramework/Character.h"

// Sets default values
AFallingPlatform::AFallingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AFallingPlatform::BeginPlay()
{
	Super::BeginPlay();
	
}

void AFallingPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFallingPlatform::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	if (Other->IsA(ACharacter::StaticClass()))
	{
		if (HitNormal.Z >= 0.0f)
		{
			BeginFalling();
		}
	}
}

void AFallingPlatform::BeginFalling()
{
}
