// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "Conveyor.h"


UConveyorComponent::UConveyorComponent()
{
#if WITH_EDITOR
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Direction"));
	ArrowComponent->SetupAttachment(this);
	ArrowComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ArrowComponent->SetRelativeRotation(BeltVelocity.ToOrientationQuat());
	ArrowComponent->ArrowLength = BeltVelocity.Size();
#endif
}

void UConveyorComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
}

void UConveyorComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
#if WITH_EDITOR
	const FName PropertyName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UConveyorComponent, BeltVelocity))
	{
		ArrowComponent->SetRelativeRotation(BeltVelocity.ToOrientationQuat());
		ArrowComponent->ArrowLength = BeltVelocity.Size();
	}
#endif
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

// Called when the game starts or when spawned
void UConveyorComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

FVector UConveyorComponent::GetAppliedVelocity() const
{
	return GetComponentRotation().RotateVector(BeltVelocity);
}

