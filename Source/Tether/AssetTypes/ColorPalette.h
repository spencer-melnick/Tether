#pragma once

#include "CoreMinimal.h"

#include "ColorPalette.generated.h"

UCLASS(BlueprintType, HideCategories = "Object")
class UColorPaletteAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	const FColor& GetColorByIndex(const int Index);

	UPROPERTY(EditDefaultsOnly)
	TArray<FColor> Colors;

private:
	const FColor EmptyColor = FColor::Transparent;
	FName AssetName;
};
