// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherSimpleButton.h"


UTetherSimpleButton::UTetherSimpleButton()
{
	Cursor = EMouseCursor::Hand;
}

void UTetherSimpleButton::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	SetFocus();
}

FReply UTetherSimpleButton::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FReply Reply = Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	
	if (IsFocused() && !Reply.IsEventHandled() && FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Accept)
	{
		OnButtonPressed.Broadcast();
		return FReply::Handled();
	}

	return Reply;
}

FReply UTetherSimpleButton::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	if (IsFocused() && !Reply.IsEventHandled() && (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || InMouseEvent.IsTouchEvent()))
	{
		OnButtonPressed.Broadcast();
		return FReply::Handled();
	}

	return Reply;
}


