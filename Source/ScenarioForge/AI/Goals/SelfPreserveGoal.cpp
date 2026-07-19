// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file SelfPreserveGoal.cpp
 * @brief Implements the utility-driven SelfPreserve goal.
 */

#include "SelfPreserveGoal.h"

#include "ScenarioForgeGameplayTags.h"

/**
 * @brief Configures the self-preservation desired state and default importance.
 */
USelfPreserveGoal::USelfPreserveGoal()
{
	TrueStates.AddTag(TAG_State_SelfPreserve.GetTag());
}

/**
 * @brief Calculates SelfPreserve utility from any active danger state.
 *
 * @param CurrentStates Current true world-state tags.
 * @return Score when State.Danger or one of its descendants is present; otherwise zero.
 */
float USelfPreserveGoal::GetUtility_Implementation(const FGameplayTagContainer& CurrentStates) const
{
	return CurrentStates.HasTag(TAG_State_Danger.GetTag()) ? Score : 0.0f;
}
