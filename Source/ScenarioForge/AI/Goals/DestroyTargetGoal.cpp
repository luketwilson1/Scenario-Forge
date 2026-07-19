// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file DestroyTargetGoal.cpp
 * @brief Implements the utility-driven DestroyTarget goal.
 */

#include "DestroyTargetGoal.h"

#include "ScenarioForgeGameplayTags.h"

/**
 * @brief Configures the destroy-target desired state and default importance.
 */
UDestroyTargetGoal::UDestroyTargetGoal()
{
	TrueStates.AddTag(TAG_State_DestroyTarget.GetTag());
}

/**
 * @brief Calculates DestroyTarget utility from current enemy visibility.
 *
 * @param CurrentStates Current true world-state tags.
 * @return Score when the agent sees an enemy; otherwise zero.
 */
float UDestroyTargetGoal::GetUtility_Implementation(const FGameplayTagContainer& CurrentStates) const
{
	return CurrentStates.HasTagExact(TAG_State_SeesEnemy.GetTag()) ? Score : 0.0f;
}
