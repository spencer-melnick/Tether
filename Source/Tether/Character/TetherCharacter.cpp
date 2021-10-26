// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherCharacter.h"

#include "DrawDebugHelpers.h"
#include "MovementComponent/PupMovementComponent.h"
#include "Algo/Transform.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tether/Tether.h"
#include "Tether/GameMode/TetherPrimaryGameMode.h"
#include "Tether/GameMode/TetherPrimaryGameState.h"
#include "Tether/Gameplay/Cameras/TopDownCameraComponent.h"


// Component name constants

const FName ATetherCharacter::CapsuleComponentName(TEXT("CapsuleComponent"));
const FName ATetherCharacter::MovementComponentName(TEXT("CharacterMovementComponent"));
const FName ATetherCharacter::SkeletalMeshComponentName(TEXT("SkeletalMesh"));
const FName ATetherCharacter::GrabSphereComponentName(TEXT("GrabSphere"));
const FName ATetherCharacter::GrabHandleName(TEXT("GrabHandle"));
const FName ATetherCharacter::MantleHandleName(TEXT("MantleHandle"));
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

	MantleLocationComponent = CreateDefaultSubobject<USceneComponent>(MantleHandleName);
	MantleLocationComponent->SetupAttachment(RootComponent);
	MantleLocationComponent->ComponentTags.Add(TEXT("MantleHandle"));

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
	PlayerInputComponent->BindAction(TEXT("Grab"), EInputEvent::IE_Released, this, &ATetherCharacter::Release);

	PlayerInputComponent->BindAxis(TEXT("RotateX"), this, &ATetherCharacter::RotateX);
	PlayerInputComponent->BindAxis(TEXT("RotateY"), this, &ATetherCharacter::RotateY);
}


void ATetherCharacter::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent->OnForceDragReleaseEvent().AddWeakLambda(this, [this]
	{
		// The Movement component has initiated the break, so we *must not* instruct it to do anything else
		// and trust that it has already handled the necessary logic.
		// Otherwise, the event just becomes a circular reference
		bDraggingObject = false;
		MovementComponent->UnignoreActor(DraggingActorObject);
		DraggingActorObject->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	});
}


void ATetherCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (SkeletalMeshComponent->IsPlayingRootMotion())
	{
		const FRootMotionMovementParams RootMotion = SkeletalMeshComponent->ConsumeRootMotion();
		if (RootMotion.bHasRootMotion)
		{
			const FTransform WorldSpaceRootMotionTransform = SkeletalMeshComponent->ConvertLocalRootMotionToWorld( RootMotion.GetRootMotionTransform() );
			// UE_LOG(LogTetherGame, Verbose, TEXT("Root motion occuring! %s"), *WorldSpaceRootMotionTransform.ToString());
			MovementComponent->AddRootMotionTransform(WorldSpaceRootMotionTransform);
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



void ATetherCharacter::Suspend()
{
	MovementComponent->SetComponentTickEnabled(false);
	SkeletalMeshComponent->bPauseAnims = true;
}

void ATetherCharacter::Unsuspend()
{
	MovementComponent->SetComponentTickEnabled(true);
	SkeletalMeshComponent->bPauseAnims = false;
}

void ATetherCharacter::CacheInitialState()
{
	InitialState = FPupMovementComponentState(MovementComponent);
}

void ATetherCharacter::Reload()
{
	MovementComponent->ResetState(&InitialState);
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
					if (!bCarryingObject)
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
				if (Actor->ActorHasTag(TEXT("Draggable")) && MovementComponent->bGrounded)
				{
					if (UPrimitiveComponent* Component = Cast<UPrimitiveComponent>(Actor->GetComponentByClass(UPrimitiveComponent::StaticClass())))
					{
						const FVector Direction = (Actor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
						FHitResult LineTraceResult;
						if (Component->LineTraceComponent(LineTraceResult, GetActorLocation(), GetActorLocation() + Direction * 100.0f, FCollisionQueryParams::DefaultQueryParam))
						{
							DragObject(Actor, LineTraceResult.Normal);
						}
					}
					break;
				}
			}
		}
	}
}


void ATetherCharacter::Release()
{
	if (MovementComponent->GetMovementMode() == EPupMovementMode::M_Anchored)
	{
		MovementComponent->BreakAnchor();
	}
	else if (bDraggingObject)
	{
		DragObjectRelease();
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
		Release();
	}
	
	if (!bLaunchVertically)
	{
		DeflectionNormal.Z = 0.f;
		DeflectionNormal.Normalize();
	}

	// Calculate the character's velocity relative to the instigator
	const FVector RelativeVelocity = (MovementComponent->Velocity - InstigatorVelocity) * InstigatorFactor;

	// Limit the player's velocity along the deflection normal
	const FVector NormalVelocity = FVector::DotProduct(DeflectionNormal, RelativeVelocity) * DeflectionNormal;
	const FVector ClampedVelocity = RelativeVelocity -  NormalVelocity;

	// Add velocity equal to the deflection scale plus the elastic contribution
	// Clamp elastic contribution in one direction
	// const float BounceFactor = DeflectionScale - (NormalVelocity * Elasticity);
	// const FVector BounceVelocity = ClampedVelocity + (DeflectionNormal * BounceFactor);

	// Limit velocity by the max launch speed and then launch
	// const FVector FinalVelocity = BounceVelocity.GetClampedToSize(0.f, MaxLaunchSpeed);
	FVector FinalVelocity = ClampedVelocity;

	#if WITH_EDITOR
	if (GEngine->GameViewport && GEngine->GameViewport->EngineShowFlags.Collision)
	{
		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + FinalVelocity / 10.0f, 25.0f, FColor::Blue, false, 3.0f, 0, 5.0f);
	}
	#endif
				
	
	// MovementComponent->AddImpulse(FinalVelocity);
	MovementComponent->Deflect(FinalVelocity, DeflectTime);
}

void ATetherCharacter::DeflectSimple(const FVector Velocity, float DeflectTime)
{
	MovementComponent->Deflect(Velocity, DeflectTime);
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
	CarriedActor = Object;
	InitialCarriedActorPosition = Object->GetActorLocation();
	InitialCarriedActorRotation = Object->GetActorRotation();
	if (UPrimitiveComponent* Target = Cast<UPrimitiveComponent>(CarriedActor->GetRootComponent()))
	{
		MovementComponent->IgnoreActor(Object);
		// Target->SetSimulatePhysics(false);
		// Target->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		Object->AttachToComponent(RootComponent, FAttachmentTransformRules(
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::KeepWorld,
			true));
		const float CarriedObjectHeight = Target->Bounds.BoxExtent.Z;
		Object->AddActorLocalOffset(FVector(0.0f, 0.0f, CarriedObjectHeight + CapsuleComponent->GetScaledCapsuleHalfHeight()));
	}
}


void ATetherCharacter::DropObject()
{
	bCarryingObject = false;
	bCompletedPickupAnimation = false;
	SnapFactor = 0.0f;
	if(CarriedActor)
	{
		UPrimitiveComponent* Target = Cast<UPrimitiveComponent>(CarriedActor->GetRootComponent());
		if (Target)
		{
			const float CarriedObjectDepth = Target->Bounds.BoxExtent.X;
			Target->SetSimulatePhysics(true);

			CarriedActor->AddActorLocalOffset(FVector(CarriedObjectDepth * 2.0f, 0.0f, 0.0f));
		}
		MovementComponent->UnignoreActor(CarriedActor);
		CarriedActor->DetachFromActor(FDetachmentTransformRules(
			EDetachmentRule::KeepWorld,
			EDetachmentRule::KeepWorld,
			EDetachmentRule::KeepWorld,
			true));
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


void ATetherCharacter::DragObject(AActor* Target, const FVector Normal)
{
	bDraggingObject = true;
	MovementComponent->BeginDraggingObject(Normal);
	MovementComponent->IgnoreActor(Target);
	DraggingActorObject = Target;
	FAttachmentTransformRules AttachmentTransformRules = FAttachmentTransformRules::KeepWorldTransform;
	AttachmentTransformRules.bWeldSimulatedBodies = true;
	Target->AttachToActor(this, AttachmentTransformRules);
}


void ATetherCharacter::DragObjectRelease()
{
	MovementComponent->EndDraggingObject();
	MovementComponent->UnignoreActor(DraggingActorObject);
	bDraggingObject = false;
	DraggingActorObject->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}
