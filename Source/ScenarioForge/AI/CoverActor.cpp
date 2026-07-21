// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AI/CoverActor.cpp
 * @brief Implements the native cover actor and its peek-point selection.
 */

#include "CoverActor.h"

#include "Components/ArrowComponent.h"

namespace
{
	UArrowComponent* FindArrow(const ACoverActor* CoverActor, const FName Name)
	{
		TInlineComponentArray<UArrowComponent*> Arrows(CoverActor);
		for (UArrowComponent* Arrow : Arrows)
		{
			if (Arrow && Arrow->GetFName() == Name)
			{
				return Arrow;
			}
		}
		return nullptr;
	}
}

void ACoverActor::GetEnabledPeekPoints(TArray<UArrowComponent*>& OutPeekPoints) const
{
	OutPeekPoints.Reset();
	if ((CoverType & static_cast<int32>(ECoverType::Crouch)) != 0)
	{
		if (UArrowComponent* Arrow = FindArrow(this, TEXT("CrouchPeek")))
		{
			OutPeekPoints.Add(Arrow);
		}
	}
	if ((CoverType & static_cast<int32>(ECoverType::Left)) != 0)
	{
		if (UArrowComponent* Arrow = FindArrow(this, TEXT("LeftPeek")))
		{
			OutPeekPoints.Add(Arrow);
		}
	}
	if ((CoverType & static_cast<int32>(ECoverType::Right)) != 0)
	{
		if (UArrowComponent* Arrow = FindArrow(this, TEXT("RightPeek")))
		{
			OutPeekPoints.Add(Arrow);
		}
	}
}
