// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "FallingPlatform.h"

#include "GameFramework/Character.h"
#include "Tether/Character/TetherCharacter.h"

// Sets default values
AFallingPlatform::AFallingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}


void AFallingPlatform::BeginPlay()
{
	Super::BeginPlay();
	InitialLocation = GetActorLocation();
	InitialRotation = GetActorRotation();
	for (UActorComponent* Component : GetComponents())
	{
		UpdatedComponents.Add(Cast<UPrimitiveComponent>(Component));	
	}	
}


void AFallingPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bIsFalling)
	{
		Velocity.Z += GetWorld()->GetGravityZ() * DeltaTime * DeltaTime;
		AddActorWorldOffset(Velocity);
	}
}


void AFallingPlatform::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsSuspended)
	{
		return;
	}
	if (Other->IsA(ATetherCharacter::StaticClass()))
	{
		if (HitNormal.Z >= 0.0f)
		{
			Destabilize();
		}
	}
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
}




void AFallingPlatform::Suspend()
{
	PrimaryActorTick.SetTickFunctionEnable(false);
	bIsSuspended = true;
	GetWorld()->GetTimerManager().PauseTimer(FallTimerHandle);
}

void AFallingPlatform::Unsuspend()
{
	if (bIsFalling || bIsShaking)
	{
		PrimaryActorTick.SetTickFunctionEnable(true);
	}
	bIsSuspended = false;
	GetWorld()->GetTimerManager().UnPauseTimer(FallTimerHandle);
}

void AFallingPlatform::CacheInitialState()
{
	// we already do this as part of our respawn logic!
}

void AFallingPlatform::Reload()
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	SetActorLocation(InitialLocation);
	Velocity = FVector::ZeroVector;
	bIsFalling = false;
}


void AFallingPlatform::Destabilize()
{
	if (bIsShaking || bIsFalling)
	{
		return;
	}
	if (ShakeTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(FallTimerHandle, FTimerDelegate::CreateUObject(this, &AFallingPlatform::BeginFalling), ShakeTime, false);
	}
	bIsShaking = true;
	PrimaryActorTick.SetTickFunctionEnable(true);
	StartShaking();
}


void AFallingPlatform::BeginFalling()
{
	bIsShaking = false;
	bIsFalling = true;
	GetWorld()->GetTimerManager().SetTimer(FallTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		this->Despawn();
	}), FallTime, false);
}

void AFallingPlatform::Despawn()
{
	if (bRespawn)
	{
		UWorld* World = GetWorld();
		UClass* Class = GetClass();

		const FVector Location = InitialLocation;
		const FRotator Rotation = InitialRotation;
		GetWorld()->GetTimerManager().SetTimer(FallTimerHandle, FTimerDelegate::CreateWeakLambda(World, [Class, World, Location, Rotation]
		{
			World->SpawnActor(Class, &Location, &Rotation);
		}), FallTime, false);
	}
	Destroy();
}
