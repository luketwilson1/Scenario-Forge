// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file DestroyTargetGoal.h
 * @brief Declares the goal that asks the planner to destroy its current target.
 */

#pragma once

#include "CoreMinimal.h"
#include "Goal.h"
#include "DestroyTargetGoal.generated.h"

/**
 * @brief Goal satisfied when State.DestroyTarget is true.
 */
UCLASS(BlueprintType, meta = (DisplayName = "DestroyTarget"))
class SCENARIOFORGE_API UDestroyTargetGoal : public UGoal
{
	GENERATED_BODY()

public:
	/** @brief Initializes State.DestroyTarget. Utility is assigned from the Agent Sheet. */
	UDestroyTargetGoal();

	/**
	 * @brief Returns utility only while this agent can see an enemy.
	 *
	 * @param CurrentStates Current true world-state tags.
	 * @return Score when State.SeesEnemy is true; otherwise zero.
	 */
	virtual float GetUtility_Implementation(const FGameplayTagContainer& CurrentStates) const override;
};
