// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file IdleActionBehavior.cpp
 * @brief Implements the idle GOAP action behavior.
 */

#include "IdleActionBehavior.h"

#include "DecisionComponent.h"
#include "ScenarioForgeGameplayTags.h"

/**
 * @brief Marks the executing decision component as idle.
 *
 * @param Agent Decision component executing the idle behavior.
 * @param ActionDefinition Action definition associated with this behavior.
 */
void UIdleActionBehavior::Execute_Implementation(UDecisionComponent* Agent, const UActionDefinition* ActionDefinition)
{
	if (!Agent)
	{
		return;
	}

	Agent->AddCurrentState(TAG_State_Idle.GetTag());
}
