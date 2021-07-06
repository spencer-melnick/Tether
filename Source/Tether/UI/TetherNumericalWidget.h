// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "TetherUserWidget.h"
#include "TetherNumericalWidget.generated.h"


class UTextBlock;
class UButton;


/**
 * Simple widget that lets the user select a number via keyboard navigation or buttons
 */
UCLASS()
class UTetherNumericalWidget : public UTetherUserWidget
{
	GENERATED_BODY()

public:

	UTetherNumericalWidget();
	

	// User widget interface

	virtual void OnWidgetRebuilt() override;
	virtual void NativeOnInitialized() override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent, const FNavigationReply& InDefaultReply) override;
	

	// Accessors

	UFUNCTION(BlueprintPure)
	int GetNumberValue() const { return NumberValue; }

	void SetNumberValue(int NewNumberValue);


	// Event handles

	UFUNCTION()
	void HandleLeftButtonPressed();

	UFUNCTION()
	void HandleRightButtonPressed();
	

	// Delegates

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNumberChanged, int, NewNumber);

	UPROPERTY(BlueprintAssignable)
	FOnNumberChanged OnNumberChanged;


	// Widget bindings

	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	UTextBlock* NumberDisplay;

	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	UButton* LeftButton;

	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	UButton* RightButton;
	

	// Editor properties

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Number")
	int MinValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Number")
	int MaxValue = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Number")
	int DefaultValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Number")
	bool bWrapValue = true;

	/** Text format used to write number values to the the number display widget with {NumberValue} */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Number")
	FText NumberFormat;


private:

	void UpdateNumberDisplay(int DisplayNumber);

	int NumberValue = 0;
	
};
