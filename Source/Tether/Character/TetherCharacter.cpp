// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"


// Component name constants

const FName ATetherCharacter::CameraComponentName(TEXT("CameraComponent"));
const FName ATetherCharacter::SpringArmComponentName(TEXT("SpringArm"));


ATetherCharacter::ATetherCharacter()
{
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(SpringArmComponentName);
	SpringArmComponent->SetupAttachment(RootComponent);
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(CameraComponentName);
	CameraComponent->SetupAttachment(SpringArmComponent);
}


// Actor overrides

void ATetherCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveX"), this, &ATetherCharacter::MoveX);
	PlayerInputComponent->BindAxis(TEXT("MoveY"), this, &ATetherCharacter::MoveY);
	PlayerInputComponent->BindAxis(TEXT("RotateX"), this, &ATetherCharacter::RotateX);
	PlayerInputComponent->BindAxis(TEXT("RotateY"), this, &ATetherCharacter::RotateY);

	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Released, this, &ACharacter::StopJumping);
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
	AddMovementInput(GetActorRightVector(), Scale);
}

void ATetherCharacter::MoveY(float Scale)
{
	AddMovementInput(GetActorForwardVector(), Scale);
}

void ATetherCharacter::RotateX(float Scale)
{
	AddControllerYawInput(Scale);
}

void ATetherCharacter::RotateY(float Scale)
{
	AddControllerPitchInput(Scale);
}


// Accessors

FVector ATetherCharacter::GetTetherTargetLocation() const
{
	return GetActorTransform().TransformPosition(TetherOffset);
}
