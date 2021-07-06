// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "TetherUserWidget.h"
#include "TetherSimpleButton.generated.h"

/** Simple button that handles mouse click or navigation events */
UCLASS()
class UTetherSimpleButton : public UTetherUserWidget
{
	GENERATED_BODY()

public:

	UTetherSimpleButton();
	

	// User widget interface
	
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;


	// Delegates

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonPressed);

	UPROPERTY(BlueprintAssignable)
	FOnButtonPressed OnButtonPressed;
};
