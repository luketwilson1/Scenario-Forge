// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ActionBehavior.cpp
 * @brief Implements the default no-op GOAP action behavior.
 */

#include "ActionBehavior.h"

#include "ActionDefinition.h"
#include "DecisionComponent.h"

/**
 * @brief Default action execution implementation.
 *
 * Base action behaviors intentionally do nothing; concrete native or Blueprint
 * subclasses provide behavior-specific execution.
 *
 * @param Agent Decision component executing the action.
 * @param ActionDefinition Action data associated with this behavior.
 */
void UActionBehavior::Execute_Implementation(UDecisionComponent* Agent, const UActionDefinition* ActionDefinition)
{
}
