#include "ColorPalette.h"

const FColor& UColorPaletteAsset::GetColorByIndex(const int Index)
{
	if (Colors.IsValidIndex(Index))
	{
		return Colors[Index];
	}
	return EmptyColor;
}
