// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherCharacter.h"

#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Tether/GameMode/TetherPrimaryGameMode.h"


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
	
	// CameraComponent = CreateDefaultSubobject<UCameraComponent>(CameraComponentName);
	// CameraComponent->SetupAttachment(SpringArmComponent);

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

void ATetherCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement())
	{
		CharacterMovementComponent->GroundFriction = NormalFriction;
	}
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
	if (CanMove())
	{
		FRotator ForwardDirection;
		FVector Location;
		GetController()->GetPlayerViewPoint(Location, ForwardDirection );
		
		const FRotator YawRotator = FRotator(0.f, ForwardDirection.Yaw, 0.f);
		AddMovementInput(YawRotator.RotateVector(FVector::RightVector), Scale);
	}
}

void ATetherCharacter::MoveY(float Scale)
{
	if (CanMove())
	{
		FRotator ForwardDirection;
		FVector Location;
		GetController()->GetPlayerViewPoint(Location, ForwardDirection );
		
		const FRotator YawRotator = FRotator(0.f, ForwardDirection.Yaw, 0.f);
		AddMovementInput(YawRotator.RotateVector(FVector::ForwardVector), Scale);
	}
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
			UStaticMeshComponent* Target = Cast<UStaticMeshComponent>(GrabbedObject->GetRootComponent());
			if (Target)
			{
				Target->SetSimulatePhysics(false);
				Target->SetCollisionProfileName(TEXT("IgnoreOnlyPawn"));
				Target->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
			}
			Closest->AttachToComponent(GrabHandle, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}

	else if(bCarryingObject)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.f, FColor::Black, TEXT("Removed object"), true, FVector2D(1,1));
		TArray<USceneComponent*> ChildComponents;
		GrabHandle->GetChildrenComponents(false, ChildComponents);
		GrabbedObject->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		UStaticMeshComponent* Target = Cast<UStaticMeshComponent>(GrabbedObject->GetRootComponent());
		if (Target)
		{
			Target->SetCollisionProfileName(TEXT("PhysicsActor"));
			Target->SetSimulatePhysics(true);
			bCarryingObject = false;
		}
	}
}

void ATetherCharacter::SetGroundFriction(float GroundFriction)
{
	if (UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement())
	{
		CharacterMovementComponent->GroundFriction = GroundFriction;
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

// ReSharper disable once CppMemberFunctionMayBeStatic
bool ATetherCharacter::CanMove() const
{
	return true;
}

void ATetherCharacter::Deflect(
	FVector DeflectionNormal, float DeflectionScale,
	FVector InstigatorVelocity, float InstigatorFactor,
	float Elasticity, float DeflectTime, bool bLaunchVertically)
{	
	if (const UWorld* World = GetWorld())
	{
		SetGroundFriction(BounceFriction);
		bIsBouncing = true;
		
		// Only restart the timer if the remaining time would increase
		const float DeflectTimeRemaining = DeflectTimerHandle.IsValid() ? World->GetTimerManager().GetTimerRemaining(DeflectTimerHandle) : 0.f;
		if (DeflectTimeRemaining < DeflectTime)
		{
			World->GetTimerManager().SetTimer(DeflectTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]
			{
				SetGroundFriction(NormalFriction);
				bIsBouncing = false;
			}), DeflectTime, false);
		}
	}

	if (!bLaunchVertically)
	{
		DeflectionNormal.Z = 0.f;
		DeflectionNormal.Normalize();
	}

	// Calculate the character's velocity relative to the instigator
	const FVector RelativeVelocity = GetVelocity() - (InstigatorVelocity * InstigatorFactor);

	// Limit the player's velocity along the deflection normal
	const float NormalVelocity = FMath::Min(0.f, FVector::DotProduct(DeflectionNormal, RelativeVelocity));
	const FVector ClampedVelocity = RelativeVelocity - NormalVelocity * DeflectionNormal;

	// Add velocity equal to the deflection scale plus the elastic contribution
	// Clamp elastic contribution in one direction
	const float BounceFactor = DeflectionScale - (NormalVelocity * Elasticity);
	const FVector BounceVelocity = ClampedVelocity + (DeflectionNormal * BounceFactor);

	// Limit velocity by the max launch speed and then launch
	const FVector FinalVelocity = BounceVelocity.GetClampedToSize(0.f, MaxLaunchSpeed);
	LaunchCharacter(FinalVelocity, true, true);
}


// Events

void ATetherCharacter::OnTetherExpired()
{
	bAlive = false;

	UCharacterMovementComponent* MovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
	if (MovementComponent)
	{
		MovementComponent->SetMovementMode(MOVE_None);
	}
}
