// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"


// Component name constants

const FName ATetherCharacter::CameraComponentName(TEXT("CameraComponent"));
const FName ATetherCharacter::SpringArmComponentName(TEXT("SpringArm"));
const FName ATetherCharacter::GrabSphereComponentName(TEXT("GrabSphere"));
const FName ATetherCharacter::GrabHandleName(TEXT("GrabHandle"));

const FName ATetherCharacter::PickupTag(TEXT("Pickup"));


ATetherCharacter::ATetherCharacter()
{
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(SpringArmComponentName);
	SpringArmComponent->SetupAttachment(RootComponent);
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(CameraComponentName);
	CameraComponent->SetupAttachment(SpringArmComponent);

	GrabSphereComponent = CreateDefaultSubobject<USphereComponent>(GrabSphereComponentName);
	GrabSphereComponent->SetupAttachment(RootComponent);

	GrabHandle = CreateDefaultSubobject<USceneComponent>(GrabHandleName);
	GrabHandle->SetupAttachment(RootComponent);
}


// Actor overrides

void ATetherCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveX"), this, &ATetherCharacter::MoveX);
	PlayerInputComponent->BindAxis(TEXT("MoveY"), this, &ATetherCharacter::MoveY);
	// PlayerInputComponent->BindAxis(TEXT("RotateX"), this, &ATetherCharacter::RotateX);
	// PlayerInputComponent->BindAxis(TEXT("RotateY"), this, &ATetherCharacter::RotateY);

	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction(TEXT("Grab"), EInputEvent::IE_Pressed, this, &ATetherCharacter::GrabObject);
}


// Character overrides

bool ATetherCharacter::CanJumpInternal_Implementation() const
{
	return bCoyoteJumpAvailable || Super::CanJumpInternal_Implementation();
}

void ATetherCharacter::Falling()
{
	Super::Falling();

	if (FMath::IsNearlyZero(CoyoteTime))
	{
		bCoyoteJumpAvailable = false;
	}
	else
	{
		if (const UWorld* World = GetWorld())
		{
			bCoyoteJumpAvailable = true;
			
			World->GetTimerManager().SetTimer(CoyoteJumpTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
			{
				bCoyoteJumpAvailable = false;
			}), CoyoteTime, false);
		}
	}
}

void ATetherCharacter::OnJumped_Implementation()
{
	if (bCoyoteJumpAvailable)
	{
		bCoyoteJumpAvailable = false;

		if (const UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CoyoteJumpTimerHandle);
		}
	}
}


// Movement functions

void ATetherCharacter::MoveX(float Scale)
{
	FRotator ForwardDirection;
	FVector Location;
	GetController()->GetPlayerViewPoint(Location, ForwardDirection );
	AddMovementInput(ForwardDirection.RotateVector(FVector::RightVector), Scale);
}

void ATetherCharacter::MoveY(float Scale)
{
	FRotator ForwardDirection;
	FVector Location;
	GetController()->GetPlayerViewPoint(Location, ForwardDirection );
	AddMovementInput(ForwardDirection.RotateVector(FVector::ForwardVector), Scale);
}

void ATetherCharacter::RotateX(float Scale)
{
	AddControllerYawInput(Scale);
}

void ATetherCharacter::RotateY(float Scale)
{
	AddControllerPitchInput(Scale);
}

void ATetherCharacter::GrabObject()
{
	// if object inside collision box and not holding anything, pick it up.
	if(!bCarryingObject)
	{
		TArray<AActor*> Overlaps;
		GrabSphereComponent->GetOverlappingActors(Overlaps);
		if(Overlaps.Num() <= 0)
		{
			return;
		}
		AActor* Closest = Overlaps[0];
		float MinimumDist = 1000;
		bool bValid = false;
		for (AActor* Actor: Overlaps)
		{
			if(Actor->ActorHasTag(PickupTag))
			{
				const float Dist = GetDistanceTo(Actor);
				if (Dist < MinimumDist)
				{
					MinimumDist = Dist;
					Closest = Actor;
					bValid = true;
				}
			}
		}
		if(bValid)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.f, FColor::Black, TEXT("Attached object"), true, FVector2D(1,1));
			bCarryingObject = true;
			GrabbedObject = Closest;
			((UStaticMeshComponent*) GrabbedObject->GetRootComponent())->SetSimulatePhysics(false);
			Closest->AttachToComponent(GrabHandle, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}

	else if(bCarryingObject)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.f, FColor::Black, TEXT("Removed object"), true, FVector2D(1,1));
		TArray<USceneComponent*> ChildComponents;
		GrabHandle->GetChildrenComponents(false, ChildComponents);
		GrabbedObject->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		((UStaticMeshComponent*) GrabbedObject->GetRootComponent())->SetSimulatePhysics(true);
		bCarryingObject = false;
	}
}



// Accessors

FVector ATetherCharacter::GetTetherTargetLocation() const
{
	return GetActorTransform().TransformPosition(TetherOffset);
}

FVector ATetherCharacter::GetTetherEffectLocation() const
{
	const USkeletalMeshComponent* MeshComponent = GetMesh();
	if (MeshComponent && MeshComponent->DoesSocketExist(TetherEffectSocket))
	{
		return MeshComponent->GetSocketLocation(TetherEffectSocket);
	}
	
	return GetTetherTargetLocation();
}
