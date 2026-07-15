// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file KillTargetGoal.h
 * @brief Declares the goal that asks the planner to kill the current target.
 */

#pragma once

#include "CoreMinimal.h"
#include "Goal.h"
#include "KillTargetGoal.generated.h"

/**
 * @brief Goal satisfied when State.CurrentTargetDead is true.
 */
UCLASS(BlueprintType, meta = (DisplayName = "KillTarget"))
class SCENARIOFORGE_API UKillTargetGoal : public UGoal
{
	GENERATED_BODY()

public:
	/** @brief Initializes the State.CurrentTargetDead true state. */
	UKillTargetGoal();
};
