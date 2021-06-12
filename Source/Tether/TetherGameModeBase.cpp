// Copyright Epic Games, Inc. All Rights Reserved.


#include "TetherGameModeBase.h"

#include "Character/TetherCharacter.h"

ATetherGameModeBase::ATetherGameModeBase()
{
	DefaultPawnClass = ATetherCharacter::StaticClass();
}
