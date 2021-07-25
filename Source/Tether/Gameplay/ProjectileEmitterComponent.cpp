// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "ProjectileEmitterComponent.h"

#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Tether/Gameplay/SimpleProjectile.h"

// Sets default values for this component's properties
UProjectileEmitterComponent::UProjectileEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
#if WITH_EDITOR
	UArrowComponent* Direction = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Direction"));
	Direction->SetupAttachment(this);
	Direction->SetCollisionEnabled(ECollisionEnabled::NoCollision); 

	USphereComponent* Sphere = CreateEditorOnlyDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(this);
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); 
#endif
}


// Called when the game starts
void UProjectileEmitterComponent::BeginPlay()
{
	Super::BeginPlay();
	
}


// Called every frame
void UProjectileEmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}


void UProjectileEmitterComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.GetPropertyName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UProjectileEmitterComponent, ProjectileType))
	{
		FVector ProjectileOrigin;
		FVector ProjectileExtent;
		ProjectileType.GetDefaultObject()->GetActorBounds(true, ProjectileOrigin, ProjectileExtent);
		ProjectileRadius = FMath::Abs(ProjectileOrigin.X - ProjectileExtent.X) / 2;
	}
}


void UProjectileEmitterComponent::FireProjectile()
{
	if (UWorld* World = GetWorld())
	{
		if (ProjectileType && ProjectileLifetime > 0 && !isnan(ProjectileLifetime))
		{
			ASimpleProjectile* Projectile = World->SpawnActor<ASimpleProjectile>(ProjectileType, GetComponentLocation(), GetComponentRotation());
			Projectile->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
			Projectile->Velocity = GetForwardVector() * ProjectileVelocity;
			Projectile->SetLifeSpan(ProjectileLifetime);
		}
	}
}
