// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FindCoverGoal.h
 * @brief Declares the goal that asks the planner to place an agent in cover.
 */

#pragma once

#include "CoreMinimal.h"
#include "Goal.h"
#include "FindCoverGoal.generated.h"

/**
 * @brief Goal satisfied when State.InCover is true.
 */
UCLASS(BlueprintType, meta = (DisplayName = "FindCover"))
class SCENARIOFORGE_API UFindCoverGoal : public UGoal
{
	GENERATED_BODY()

public:
	/** @brief Initializes the State.InCover true state. */
	UFindCoverGoal();
};
