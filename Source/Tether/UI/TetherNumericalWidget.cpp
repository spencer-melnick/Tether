// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#include "TetherNumericalWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"


UTetherNumericalWidget::UTetherNumericalWidget()
{
	NumberFormat = NSLOCTEXT("TetherUI", "DefaultNumberFormat", "{NumberValue}");
}

void UTetherNumericalWidget::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();

	UpdateNumberDisplay(DefaultValue);

	if (LeftButton && RightButton)
	{
		LeftButton->IsFocusable = false;
		RightButton->IsFocusable = false;
	}
}

void UTetherNumericalWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetNumberValue(DefaultValue);

	if (LeftButton && RightButton)
	{
		LeftButton->IsFocusable = false;
		RightButton->IsFocusable = false;
		
		LeftButton->OnClicked.AddUniqueDynamic(this, &UTetherNumericalWidget::HandleLeftButtonPressed);
		RightButton->OnClicked.AddUniqueDynamic(this, &UTetherNumericalWidget::HandleRightButtonPressed);
	}
}

FNavigationReply UTetherNumericalWidget::NativeOnNavigation(const FGeometry& MyGeometry,
	const FNavigationEvent& InNavigationEvent, const FNavigationReply& InDefaultReply)
{
	switch (InNavigationEvent.GetNavigationType())
	{
		case EUINavigation::Left:
			HandleLeftButtonPressed();
			return FNavigationReply::Stop();

		case EUINavigation::Right:
			HandleRightButtonPressed();
			return FNavigationReply::Stop();

		default:
			return Super::NativeOnNavigation(MyGeometry, InNavigationEvent, InDefaultReply);
	}
}

void UTetherNumericalWidget::SetNumberValue(int NewNumberValue)
{
	if (bWrapValue)
	{
		const int Range = MaxValue - MinValue + 1;

		while (NewNumberValue > MaxValue)
		{
			NewNumberValue -= Range;
		}
		while (NewNumberValue < MinValue)
		{
			NewNumberValue += Range;
		}
	}
	else
	{
		NewNumberValue = FMath::Clamp(NewNumberValue, MinValue, MaxValue);
	}

	NumberValue = NewNumberValue;
	UpdateNumberDisplay(NumberValue);
	OnNumberChanged.Broadcast(NumberValue);
}

void UTetherNumericalWidget::HandleLeftButtonPressed()
{
	SetNumberValue(NumberValue - 1);
}

void UTetherNumericalWidget::HandleRightButtonPressed()
{
	SetNumberValue(NumberValue + 1);
}

void UTetherNumericalWidget::UpdateNumberDisplay(int DisplayNumber)
{
	if (NumberDisplay)
	{
		NumberDisplay->SetText(FText::FormatNamed(NumberFormat, TEXT("NumberValue"), DisplayNumber));
	}
}
