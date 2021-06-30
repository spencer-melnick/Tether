// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "ProjectileEmitterComponent.h"

#include "Components/ArrowComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Tether/Gameplay/SimpleProjectile.h"

// Sets default values for this component's properties
UProjectileEmitterComponent::UProjectileEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	UArrowComponent* Direction = CreateDefaultSubobject<UArrowComponent>(TEXT("Direction"));
	Direction->SetupAttachment(this);
}


// Called when the game starts
void UProjectileEmitterComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UProjectileEmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UProjectileEmitterComponent::FireProjectile()
{
	if (UWorld* World = GetWorld())
	{
		if (ProjectileType)
		{
			ASimpleProjectile* Projectile = World->SpawnActor<ASimpleProjectile>(ProjectileType, GetComponentLocation(), GetComponentRotation());
			Projectile->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
			Projectile->Velocity = GetForwardVector() * ProjectileVelocity;

			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Projectile, [Projectile]
			{
				Projectile->Destroy();
			}), Lifetime, false);
		}
	}
}
