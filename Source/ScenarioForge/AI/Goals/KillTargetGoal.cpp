// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file KillTargetGoal.cpp
 * @brief Configures the native KillTarget goal defaults.
 */

#include "KillTargetGoal.h"

#include "ScenarioForgeGameplayTags.h"

/**
 * @brief Configures KillTarget to require State.CurrentTargetDead as a true goal state.
 */
UKillTargetGoal::UKillTargetGoal()
{
	TrueStates.AddTag(TAG_State_CurrentTargetDead.GetTag());
}
