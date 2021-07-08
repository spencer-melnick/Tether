// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TetherUserWidget.generated.h"

/**
 * Base class for all user widgets in the Tether game. Adds some functionality such as on focused events
 */
UCLASS()
class UTetherUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// User widget interface

	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;


	// Event handlers

	virtual void HandleFocusChanged(bool bNewFocused);
	virtual void HandleWidgetPressed();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="Handle Focus Changed"))
	void BP_HandleFocusChanged(bool bNewFocused);

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="Handle Widget Pressed"))
	void BP_HandleWidgetPressed();


	// Accessors

	UFUNCTION(BlueprintPure)
	bool IsFocused() const { return bFocused; }


	// Properties

	/** If true this widget will trigger focus events if a child widget is focused */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Focus")
	bool bCountChildFocus = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Focus")
	bool bFocusOnMouseover = true;
	

private:

	void UpdateFocus(bool bNewFocused);

	bool bFocused = false;

};
