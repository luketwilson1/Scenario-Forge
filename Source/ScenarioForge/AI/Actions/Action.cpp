// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Action.cpp
 * @brief Implements the base runtime GOAP action.
 */

#include "Action.h"

#include "Planner.h"

/**
 * @brief Provides the default no-op implementation for an action.
 *
 * @param Planner Planner that selected this runtime action.
 * @return Invalid because the abstract base action has no executable behavior.
 */
EActionResult UAction::Execute(UPlanner* Planner)
{
	return EActionResult::Invalid;
}

/**
 * @brief Declines interruption when an action has not implemented safe cleanup.
 *
 * @param Planner Planner requesting interruption.
 * @return False because the base action owns no behavior it can safely stop.
 */
bool UAction::Interrupt(UPlanner* Planner)
{
	return false;
}
