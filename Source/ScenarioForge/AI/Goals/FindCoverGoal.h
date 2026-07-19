// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FindCoverGoal.h
 * @brief Declares the goal that asks the planner to move the agent into cover.
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
	/** @brief Initializes State.InCover. Utility is assigned from the Agent Sheet. */
	UFindCoverGoal();

	/**
	 * @brief Returns utility while the agent is being fired upon and is not already in cover.
	 *
	 * @param CurrentStates Current true world-state tags.
	 * @return Score when State.FiredUpon is true and State.InCover is false; otherwise zero.
	 */
	virtual float GetUtility_Implementation(const FGameplayTagContainer& CurrentStates) const override;
};
