// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherCharacter.h"

#include "DrawDebugHelpers.h"
#include "PupMovementComponent.h"
#include "Algo/Transform.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Tether/GameMode/TetherPrimaryGameMode.h"


// Component name constants

const FName ATetherCharacter::CameraComponentName(TEXT("CameraComponent"));
const FName ATetherCharacter::SpringArmComponentName(TEXT("SpringArm"));
const FName ATetherCharacter::GrabSphereComponentName(TEXT("GrabSphere"));
const FName ATetherCharacter::GrabHandleName(TEXT("GrabHandle"));
const FName ATetherCharacter::BeamComponentName(TEXT("BeamComponent"));

const FName ATetherCharacter::PickupTag(TEXT("Pickup"));
const FName ATetherCharacter::AnchorTag(TEXT("Anchor"));


ATetherCharacter::ATetherCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPupMovementComponent>(TEXT("MovementComponent")))
{
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(SpringArmComponentName);
	SpringArmComponent->SetupAttachment(RootComponent);
	
	// CameraComponent = CreateDefaultSubobject<UCameraComponent>(CameraComponentName);
	// CameraComponent->SetupAttachment(SpringArmComponent);

	GrabSphereComponent = CreateDefaultSubobject<USphereComponent>(GrabSphereComponentName);
	GrabSphereComponent->SetupAttachment(RootComponent);

	GrabHandle = CreateDefaultSubobject<USceneComponent>(GrabHandleName);
	GrabHandle->SetupAttachment(RootComponent);

	BeamComponent = CreateDefaultSubobject<UBeamComponent>(BeamComponentName);
	BeamComponent->SetupAttachment(RootComponent);
	
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}


// Actor overrides

void ATetherCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveX"), this, &ATetherCharacter::MoveX);
	PlayerInputComponent->BindAxis(TEXT("MoveY"), this, &ATetherCharacter::MoveY);
	// PlayerInputComponent->BindAxis(TEXT("RotateX"), this, &ATetherCharacter::RotateX);
	// PlayerInputComponent->BindAxis(TEXT("RotateY"), this, &ATetherCharacter::RotateY);

	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Pressed, this, &ATetherCharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Released, this, &ATetherCharacter::StopJumping);

	PlayerInputComponent->BindAction(TEXT("Grab"), EInputEvent::IE_Pressed, this, &ATetherCharacter::Interact);
	PlayerInputComponent->BindAction(TEXT("Grab"), EInputEvent::IE_Released, this, &ATetherCharacter::ReleaseAnchor);
}

void ATetherCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ATetherCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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

void ATetherCharacter::Interact()
{
	{
		if (bCarryingObject)
		{
			DropObject();
		}
		else
		{
			TArray<AActor*> Overlaps;
			const UWorld* World = GetWorld();
			if (World && GrabSphereComponent)
			{
				TArray<FOverlapResult> OutOverlaps;
				World->OverlapMultiByChannel(OutOverlaps,
					GrabSphereComponent->GetComponentLocation(), GrabSphereComponent->GetComponentQuat(),
					InteractionTraceChannel,
					GrabSphereComponent->GetCollisionShape());

				Algo::TransformIf(OutOverlaps, Overlaps,
					[](const FOverlapResult& Overlap)
				{
					return Overlap.Actor.IsValid();
				},
					[](const FOverlapResult& Overlap)
				{
					return Overlap.Actor.Get();
				});
			}
			if(Overlaps.Num() <= 0)
			{
				return;
			}
			for (AActor* Actor: Overlaps)
			{
				if (Actor->ActorHasTag(PickupTag))
				{
					if(!bCarryingObject)
					{
						PickupObject(Actor);
						break;
					}
				}
				if (Actor->ActorHasTag(AnchorTag))
				{
					AnchorToObject(Actor);
					break;
				}
			}
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

bool ATetherCharacter::CanMove() const
{
	return !bAnchored;
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

	if (bAnchored)
	{
		ReleaseAnchor();
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

void ATetherCharacter::HandlePenetration(const FHitResult& HitResult)
{
	// Todo: accumulate worst penetration and resolve during physics update
	
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	UMovementComponent* MovementComponent = GetMovementComponent();
	
	if (GetLocalRole() >= ROLE_AutonomousProxy && Capsule && MovementComponent)
	{
		const FVector RequestedAdjustment = MovementComponent->GetPenetrationAdjustment(HitResult);
		MovementComponent->ResolvePenetration(RequestedAdjustment, HitResult, Capsule->GetComponentRotation());
	}
}

void ATetherCharacter::OnTetherExpired()
{
	bAlive = false;

	UCharacterMovementComponent* MovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
	if (MovementComponent)
	{
		MovementComponent->SetMovementMode(MOVE_None);
	}
}

void ATetherCharacter::PickupObject(AActor* Object)
{
	bCarryingObject = true;
	CarriedActor = Object;
	UPrimitiveComponent* Target = Cast<UPrimitiveComponent>(CarriedActor->GetRootComponent());
	if (Target)
	{
		Target->SetSimulatePhysics(false);
		Target->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

		FVector GrabLocation;
		if (Target->GetDistanceToCollision(GetActorLocation(), GrabLocation) > 0.f)
		{
			FVector GrabOffset = GrabLocation - Target->GetComponentLocation();
			Object->AttachToComponent(GrabHandle, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false));
			Object->AddActorWorldOffset(-GrabOffset);
		}
	}
}

void ATetherCharacter::DropObject()
{
	bCarryingObject = false;
	if(CarriedActor)
	{
		CarriedActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		UPrimitiveComponent* Target = Cast<UPrimitiveComponent>(CarriedActor->GetRootComponent());
		if (Target)
		{
			Target->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
			Target->SetSimulatePhysics(true);
		}
	}
}

void ATetherCharacter::AnchorToObject(AActor* Object)
{
	FVector ClosestPoint = GrabHandle->GetComponentLocation();

	// Search only along XY in world space
	if(UPrimitiveComponent* Target = Cast<UPrimitiveComponent>(Object->GetRootComponent()))
	{
		if(Target->GetDistanceToCollision(FVector(ClosestPoint.X, ClosestPoint.Y, Target->GetComponentLocation().Z), ClosestPoint) < 0.0f)
		{
			// If for some reason the search failed, anchor the character to their current location
			ClosestPoint = GetActorLocation();
		}
	}
	
	// GetCharacterMovement()->SetMovementMode(MOVE_Custom, static_cast<int>(ETetherMovementType::Anchored));
	const FVector Distance = ClosestPoint - GetActorLocation();
	const FRotator Rotation = Distance.ToOrientationRotator();
	ClosestPoint -= GrabHandle->GetRelativeLocation().RotateAngleAxis(Rotation.Yaw, FVector::UpVector);

	/* UTetherCharacterMovementComponent* TetherCharacterMovementComponent = static_cast<UTetherCharacterMovementComponent*>(GetCharacterMovement());
	TetherCharacterMovementComponent->SetAnchorRotation(Rotation);
	TetherCharacterMovementComponent->SetAnchorLocation(FVector(ClosestPoint.X, ClosestPoint.Y, GetActorLocation().Z));
	*/
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, *ClosestPoint.ToString());
	bAnchored = true;
}

void ATetherCharacter::ReleaseAnchor()
{
	if (bAnchored)
	{
		GetCharacterMovement()->SetDefaultMovementMode();
		bAnchored = false;
	}
}

