// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"

namespace TetherUtils
{
	/** Finds the first actor of type T that has the given actor tag */
	template <typename T = AActor>
	T* FindFirstActorWithTag(const UObject* WorldContextObject, const FName ActorTag)
	{
		UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		if (World)
		{
			for (TActorIterator<T> Iterator(World); Iterator; ++Iterator)
			{
				if (Iterator->ActorHasTag(TEXT("ObstacleVolume")))
				{
					return *Iterator;
				}
			}
		}

		return nullptr;
	}

	/** Finds all actors of type T that have the given actor tag */
	template <typename T = AActor>
	TArray<T*> FindAllActorsWithTag(const UObject* WorldContextObject, const FName ActorTag)
	{
		TArray<T*> Result;
		
		UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		if (World)
		{
			for (TActorIterator<T> Iterator(World); Iterator; ++Iterator)
			{
				if (Iterator->ActorHasTag(TEXT("ObstacleVolume")))
				{
					Result.Add(*Iterator);
				}
			}
		}

		return Result;
	}
};
