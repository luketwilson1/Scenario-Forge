// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file SelfPreserveGoal.h
 * @brief Declares the goal that asks the planner to preserve the agent from danger.
 */

#pragma once

#include "CoreMinimal.h"
#include "Goal.h"
#include "SelfPreserveGoal.generated.h"

/**
 * @brief Goal satisfied when State.SelfPreserve is true.
 */
UCLASS(BlueprintType, meta = (DisplayName = "SelfPreserve"))
class SCENARIOFORGE_API USelfPreserveGoal : public UGoal
{
	GENERATED_BODY()

public:
	/** @brief Initializes State.SelfPreserve. Utility is assigned from the Agent Sheet. */
	USelfPreserveGoal();

	/**
	 * @brief Returns utility while any State.Danger tag is true.
	 *
	 * @param CurrentStates Current true world-state tags.
	 * @return Score when any State.Danger descendant is present; otherwise zero.
	 */
	virtual float GetUtility_Implementation(const FGameplayTagContainer& CurrentStates) const override;
};
