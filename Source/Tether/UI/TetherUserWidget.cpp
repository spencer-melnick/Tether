// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherUserWidget.h"


void UTetherUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	bFocused = false;
	HandleFocusChanged(false);
}

FReply UTetherUserWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	const FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);

	UpdateFocus(true);

	return Reply;
}

void UTetherUserWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);

	UpdateFocus(false);
}

void UTetherUserWidget::NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath,
	const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
{
	static TArray<FName> SkippedWidgetTypes = {
		TEXT("SViewport"),
		TEXT("SPIEViewport")
	};
	
	if (bIsFocusable)
	{
		// Steal back focus if the viewport is trying to gain focus
		const SWidget& NewWidget = NewWidgetPath.GetLastWidget().Get();
		if (NewWidget.Advanced_IsWindow() || SkippedWidgetTypes.Contains(NewWidget.GetType()))
		{
			SetFocus();
		}
	}
}

void UTetherUserWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	
	if (bCountChildFocus)
	{
		UpdateFocus(true);
	}
}

void UTetherUserWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);

	if (bCountChildFocus)
	{
		UpdateFocus(false);
	}
}

void UTetherUserWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (bFocusOnMouseover)
	{
		SetFocus();
	}
}

FReply UTetherUserWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FReply DefaultReply = Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	
	if (!DefaultReply.IsEventHandled() && IsFocused() && FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Accept)
	{
		HandleWidgetPressed();
		return FReply::Handled();
	}

	return DefaultReply;
}

FReply UTetherUserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FReply DefaultReply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	if (!DefaultReply.IsEventHandled() && (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || InMouseEvent.IsTouchEvent()))
	{
		HandleWidgetPressed();
		return FReply::Handled();
	}

	return DefaultReply;
}

void UTetherUserWidget::HandleFocusChanged(bool bNewFocused)
{
	BP_HandleFocusChanged(bNewFocused);
}

void UTetherUserWidget::HandleWidgetPressed()
{
	BP_HandleWidgetPressed();
}

void UTetherUserWidget::UpdateFocus(bool bNewFocused)
{
	if (bFocused != bNewFocused)
	{
		bFocused = bNewFocused;
		HandleFocusChanged(bFocused);
	}
}
