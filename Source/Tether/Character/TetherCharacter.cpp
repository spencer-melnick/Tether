// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherCharacter.h"

#include "DrawDebugHelpers.h"
#include "PupMovementComponent.h"
#include "Algo/Transform.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tether/GameMode/TetherPrimaryGameMode.h"
#include "Tether/GameMode/TetherPrimaryGameState.h"


// Component name constants

const FName ATetherCharacter::CapsuleComponentName(TEXT("CapsuleComponent"));
const FName ATetherCharacter::MovementComponentName(TEXT("CharacterMovementComponent"));
const FName ATetherCharacter::SkeletalMeshComponentName(TEXT("SkeletalMesh"));
const FName ATetherCharacter::GrabSphereComponentName(TEXT("GrabSphere"));
const FName ATetherCharacter::GrabHandleName(TEXT("GrabHandle"));
const FName ATetherCharacter::BeamComponentName(TEXT("BeamComponent"));
const FName ATetherCharacter::CameraComponentName(TEXT("CameraComponent"));

const FName ATetherCharacter::PickupTag(TEXT("Pickup"));
const FName ATetherCharacter::AnchorTag(TEXT("Anchor"));


ATetherCharacter::ATetherCharacter(const FObjectInitializer& ObjectInitializer)
	//: Super(ObjectInitializer.SetDefaultSubobjectClass<UCapsuleComponent>(CapsuleComponentName))
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(CapsuleComponentName);
	RootComponent = GetCapsuleComponent();

	MovementComponent = CreateDefaultSubobject<UPupMovementComponent>(MovementComponentName);
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(SkeletalMeshComponentName);
	SkeletalMeshComponent->SetupAttachment(CapsuleComponent);
	
	GrabSphereComponent = CreateDefaultSubobject<USphereComponent>(GrabSphereComponentName);
	GrabSphereComponent->SetupAttachment(RootComponent);

	GrabHandle = CreateDefaultSubobject<USceneComponent>(GrabHandleName);
	GrabHandle->SetupAttachment(SkeletalMeshComponent, TEXT("HandleSocket"));

	BeamComponent = CreateDefaultSubobject<UBeamComponent>(BeamComponentName);
	BeamComponent->SetupAttachment(SkeletalMeshComponent);

	CameraComponent = CreateDefaultSubobject<UTopDownCameraComponent>(CameraComponentName);
	CameraComponent->SetupAttachment(RootComponent);
	
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}


// Actor overrides

void ATetherCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveX"), this, &ATetherCharacter::MoveX);
	PlayerInputComponent->BindAxis(TEXT("MoveY"), this, &ATetherCharacter::MoveY);

	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Pressed, this, &ATetherCharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Released, this, &ATetherCharacter::StopJumping);

	PlayerInputComponent->BindAction(TEXT("Grab"), EInputEvent::IE_Pressed, this, &ATetherCharacter::Interact);
	PlayerInputComponent->BindAction(TEXT("Grab"), EInputEvent::IE_Released, this, &ATetherCharacter::ReleaseAnchor);

	PlayerInputComponent->BindAxis(TEXT("RotateX"), this, &ATetherCharacter::RotateX);
	PlayerInputComponent->BindAxis(TEXT("RotateY"), this, &ATetherCharacter::RotateY);
}


void ATetherCharacter::BeginPlay()
{
	Super::BeginPlay();
}


void ATetherCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (bCarryingObject && !bCompletedPickupAnimation)
	{
		if (SnapFactor >= 1.0f)
		{
			bCompletedPickupAnimation = true;
			CarriedActor->AttachToComponent(GrabHandle, FAttachmentTransformRules(
	EAttachmentRule::SnapToTarget,
	EAttachmentRule::SnapToTarget,
	EAttachmentRule::KeepWorld, false));
		}
		else
		{
			CarriedActor->SetActorLocation(FMath::Lerp(InitialCarriedActorPosition, GrabHandle->GetComponentLocation(), SnapFactor));
			CarriedActor->SetActorRotation(FMath::Lerp(InitialCarriedActorRotation, GrabHandle->GetComponentRotation(), SnapFactor));
		}
	}
}


float ATetherCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	if (UWorld* World = GetWorld())
	{
		if (ATetherPrimaryGameState* GameState = Cast<ATetherPrimaryGameState>(World->GetGameState()))
		{
			return GameState->SubtractGlobalHealth(DamageAmount);
		}
	}
	return 0.0f;
}

void ATetherCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	PossessedDelegate.Broadcast(NewController);
}


void ATetherCharacter::Jump()
{
	if (MovementComponent->Jump())
	{
		OnJump();
	}
}


// All input functions must be non-const in order to be bound to the 'InputComponent'

// ReSharper disable CppMemberFunctionMayBeConst
void ATetherCharacter::StopJumping()
{
	MovementComponent->StopJumping();
}


void ATetherCharacter::MoveX(const float Scale)
{
	MovementComponent->AddInputVector(FVector::RightVector * Scale);
}


void ATetherCharacter::MoveY(const float Scale)
{
	MovementComponent->AddInputVector(FVector::ForwardVector * Scale);
}


void ATetherCharacter::RotateX(const float Scale)
{
	CameraComponent->AddCameraRotation(FRotator(0.0f, Scale, 0.0f));
}


void ATetherCharacter::RotateY(const float Scale)
{
	CameraComponent->AddCameraRotation(FRotator(Scale, 0.0f, 0.0f));
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


void ATetherCharacter::ReleaseAnchor()
{
	if (MovementComponent->GetMovementMode() == EPupMovementMode::M_Anchored)
	{
		MovementComponent->BreakAnchor();
	}
}
// ReSharper restore CppMemberFunctionMayBeConst



// Accessors

FVector ATetherCharacter::GetTetherTargetLocation() const
{
	return GetActorTransform().TransformPosition(TetherOffset);
}


FVector ATetherCharacter::GetTetherEffectLocation() const
{
	if (SkeletalMeshComponent && SkeletalMeshComponent->DoesSocketExist(TetherEffectSocket))
	{
		return SkeletalMeshComponent->GetSocketLocation(TetherEffectSocket);
	}
	
	return GetTetherTargetLocation();
}


bool ATetherCharacter::GetIsJumping() const
{
	if (MovementComponent)
	{
		return MovementComponent->bJumping;
	}
	return false;
}


USkeletalMeshComponent* ATetherCharacter::GetMeshComponent() const
{
	return SkeletalMeshComponent;
}


UCapsuleComponent* ATetherCharacter::GetCapsuleComponent() const
{
	return CapsuleComponent;
}


bool ATetherCharacter::CanMove() const
{
	return !bAnchored;
}

void ATetherCharacter::Deflect(
	FVector DeflectionNormal, const float DeflectionScale,
	const FVector InstigatorVelocity, const float InstigatorFactor,
	const float Elasticity, const float DeflectTime, const bool bLaunchVertically, const bool bForceBreak)
{
	if (bAnchored && bForceBreak)
	{
		ReleaseAnchor();
	}
	
	if (!bLaunchVertically)
	{
		DeflectionNormal.Z = 0.f;
		DeflectionNormal.Normalize();
	}

	// Calculate the character's velocity relative to the instigator
	const FVector RelativeVelocity = (GetVelocity() - InstigatorVelocity) * -InstigatorFactor;

	// Limit the player's velocity along the deflection normal
	const float NormalVelocity = FMath::Min(0.f, FVector::DotProduct(DeflectionNormal, RelativeVelocity));
	const FVector ClampedVelocity = RelativeVelocity - NormalVelocity * DeflectionNormal;

	// Add velocity equal to the deflection scale plus the elastic contribution
	// Clamp elastic contribution in one direction
	const float BounceFactor = DeflectionScale - (NormalVelocity * Elasticity);
	const FVector BounceVelocity = ClampedVelocity + (DeflectionNormal * BounceFactor);

	// Limit velocity by the max launch speed and then launch
	const FVector FinalVelocity = BounceVelocity.GetClampedToSize(0.f, MaxLaunchSpeed);

	#if WITH_EDITOR
	if (GEngine->GameViewport && GEngine->GameViewport->EngineShowFlags.Collision)
	{
		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + FinalVelocity / 10.0f, 25.0f, FColor::Blue, false, 3.0f, 0, 5.0f);
	}
	#endif
				
	
	// MovementComponent->AddImpulse(FinalVelocity);
	MovementComponent->Deflect(FinalVelocity, DeflectTime);
}


// Events

void ATetherCharacter::SetSnapFactor(const float Factor)
{
	if (bCarryingObject)
	{
		SnapFactor = Factor;
	}
}


void ATetherCharacter::HandlePenetration(const FHitResult& HitResult)
{
	// Todo: accumulate worst penetration and resolve during physics update
		
	if (GetLocalRole() >= ROLE_AutonomousProxy && CapsuleComponent && MovementComponent)
	{
		const FVector RequestedAdjustment = MovementComponent->GetPenetrationAdjustment(HitResult);
		MovementComponent->ResolvePenetration(RequestedAdjustment, HitResult, CapsuleComponent->GetComponentRotation());
	}
}


void ATetherCharacter::OnTetherExpired()
{
	bAlive = false;

	if (MovementComponent)
	{
		// MovementComponent->SetMovementMode(MOVE_None);
	}
}


void ATetherCharacter::PickupObject(AActor* Object)
{
	bCarryingObject = true;
	SnapFactor = 0.0f;
	CarriedActor = Object;
	InitialCarriedActorPosition = Object->GetActorLocation();
	InitialCarriedActorRotation = Object->GetActorRotation();
	if (UPrimitiveComponent* Target = Cast<UPrimitiveComponent>(CarriedActor->GetRootComponent()))
	{
		MovementComponent->IgnoreActor(Object);
		Target->SetSimulatePhysics(false);
		Target->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		Object->AttachToComponent(GrabHandle, FAttachmentTransformRules(
			EAttachmentRule::KeepWorld,
			EAttachmentRule::KeepWorld,
			EAttachmentRule::KeepWorld, false));
	}
}


void ATetherCharacter::DropObject()
{
	bCarryingObject = false;
	bCompletedPickupAnimation = false;
	SnapFactor = 0.0f;
	if(CarriedActor)
	{
		MovementComponent->UnignoreActor(CarriedActor);
		CarriedActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		UPrimitiveComponent* Target = Cast<UPrimitiveComponent>(CarriedActor->GetRootComponent());
		if (Target)
		{
			Target->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
			Target->SetSimulatePhysics(true);
		}
	}
}


void ATetherCharacter::AnchorToObject(AActor* Object) const
{
	if (UPrimitiveComponent* Component = Cast<UPrimitiveComponent>(Object->GetComponentByClass(UPrimitiveComponent::StaticClass())))
	{
		AnchorToComponent(Component);
	}
}

void ATetherCharacter::AnchorToComponent(UPrimitiveComponent* Component, const FVector& Location) const
{
	MovementComponent->AnchorToComponent(Component, Location);
}

