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


	// Event handlers

	UFUNCTION(BlueprintImplementableEvent)
	void HandleFocusChanged(bool bNewFocused);


	// Accessors

	UFUNCTION(BlueprintPure)
	bool IsFocused() const { return bFocused; }


	// Properties

	/** If true this widget will trigger focus events if a child widget is focused */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Focus")
	bool bCountChildFocus = false;
	

private:

	void UpdateFocus(bool bNewFocused);

	bool bFocused = false;

};
