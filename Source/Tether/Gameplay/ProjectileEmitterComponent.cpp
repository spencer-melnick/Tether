// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "ProjectileEmitterComponent.h"

#include "Components/ArrowComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values for this component's properties
UProjectileEmitterComponent::UProjectileEmitterComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
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
	FTimerHandle Handle;
	if (UWorld* World = GetWorld())
	{
		AActor* Projectile = World->SpawnActor<AActor>(ProjectileType, GetComponentLocation(), GetComponentRotation());
		UProjectileMovementComponent* MovementComponent = (UProjectileMovementComponent*) Projectile->GetComponentByClass(UProjectileMovementComponent::StaticClass());
		if(MovementComponent)
		{
			MovementComponent->SetVelocityInLocalSpace(FVector::ForwardVector * ProjectileVelocity);
		}
		Projectile->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [=]
			{
				if(Projectile)
				{
					Projectile->Destroy();
				}
			}), Lifetime, false);
	}
}
