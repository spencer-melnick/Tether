// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherUserWidget.h"


void UTetherUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

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

void UTetherUserWidget::UpdateFocus(bool bNewFocused)
{
	if (bFocused != bNewFocused)
	{
		bFocused = bNewFocused;
		HandleFocusChanged(bFocused);
	}
}
