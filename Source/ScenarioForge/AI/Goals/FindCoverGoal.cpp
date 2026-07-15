// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FindCoverGoal.cpp
 * @brief Configures the native FindCover goal defaults.
 */

#include "FindCoverGoal.h"

#include "ScenarioForgeGameplayTags.h"

/**
 * @brief Configures FindCover to require State.InCover as a true goal state.
 */
UFindCoverGoal::UFindCoverGoal()
{
	TrueStates.AddTag(TAG_State_InCover.GetTag());
}
