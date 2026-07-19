// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FindCoverGoal.cpp
 * @brief Implements the utility-driven FindCover goal.
 */

#include "FindCoverGoal.h"

#include "ScenarioForgeGameplayTags.h"

/**
 * @brief Configures the desired cover state.
 */
UFindCoverGoal::UFindCoverGoal()
{
	TrueStates.AddTag(TAG_State_InCover.GetTag());
}

/**
 * @brief Calculates FindCover utility from the fired-upon and cover states.
 *
 * @param CurrentStates Current true world-state tags.
 * @return Score when fired upon and not in cover; otherwise zero.
 */
float UFindCoverGoal::GetUtility_Implementation(const FGameplayTagContainer& CurrentStates) const
{
	const bool bFiredUpon = CurrentStates.HasTagExact(TAG_State_FiredUpon.GetTag());
	const bool bInCover = CurrentStates.HasTagExact(TAG_State_InCover.GetTag());
	return bFiredUpon && !bInCover ? Score : 0.0f;
}
