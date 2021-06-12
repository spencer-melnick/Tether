// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TetherCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS(Blueprintable)
class ATetherCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	// Component name constants
	
	static const FName CameraComponentName;
	static const FName SpringArmComponentName;

	
	ATetherCharacter();

	
	// Actor overrides
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	
	// Character overrides
	
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void Falling() override;
	virtual void OnJumped_Implementation() override;

	
	// Movement functions

	void MoveX(float Scale);
	void MoveY(float Scale);
	void RotateX(float Scale);
	void RotateY(float Scale);


	// Accessors

	UFUNCTION(BlueprintCallable)
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	UFUNCTION(BlueprintCallable)
	USpringArmComponent* GetSpringArmComponent() const { return SpringArmComponent; }


	// Editor properties
	
	// Jump controls

	// How long after falling a character can jump
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Jump")
	float CoyoteTime = 0.1f;

private:

	// Components
	
	UPROPERTY(VisibleAnywhere, Category="Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, Category="Components")
	USpringArmComponent* SpringArmComponent;


	// Jump tracking

	FTimerHandle CoyoteJumpTimerHandle;
	bool bCoyoteJumpAvailable = false;
	
};
