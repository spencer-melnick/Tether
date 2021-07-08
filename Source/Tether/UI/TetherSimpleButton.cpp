// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherSimpleButton.h"


UTetherSimpleButton::UTetherSimpleButton()
{
	Cursor = EMouseCursor::Hand;
}

void UTetherSimpleButton::HandleWidgetPressed()
{
	Super::HandleWidgetPressed();

	OnButtonPressed.Broadcast();
}
