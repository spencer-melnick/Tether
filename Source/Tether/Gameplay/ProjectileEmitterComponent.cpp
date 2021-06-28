// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "ProjectileEmitterComponent.h"

#include "LinearMovementComponent.h"
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
	if (UWorld* World = GetWorld())
	{
		FTimerHandle Handle;
		AActor* Projectile = World->SpawnActor<AActor>(ProjectileType, GetComponentLocation(), GetComponentRotation());
		ULinearMovementComponent* MovementComponent = (ULinearMovementComponent*) Projectile->GetComponentByClass(ULinearMovementComponent::StaticClass());
		if(MovementComponent)
		{
			MovementComponent->SetVelocity(FVector::ForwardVector * ProjectileVelocity);
		}
		Projectile->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Projectile, [Projectile]
			{
				Projectile->Destroy();
			}), Lifetime, false);
	}
}
