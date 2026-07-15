// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Idle.cpp
 * @brief Implements the idle GOAP action.
 */

#include "Idle.h"

#include "Planner.h"
#include "ScenarioForgeGameplayTags.h"

/**
 * @brief Configures the idle action's planning precondition.
 */
UIdle::UIdle()
{
	TruePreconditions.AddTag(TAG_State_Idle.GetTag());
}

/**
 * @brief Marks the executing planner as idle.
 *
 * @param Planner Planner executing the idle action.
 * @return Succeeded after applying State.Idle, or Invalid without a planner.
 */
EActionResult UIdle::Execute(UPlanner* Planner)
{
	if (!Planner)
	{
		return EActionResult::Invalid;
	}

	Planner->AddCurrentState(TAG_State_Idle.GetTag());
	return EActionResult::Succeeded;
}
