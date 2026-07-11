// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file WorldStateQuery.cpp
 * @brief Implements the default world-state query behavior.
 */

#include "WorldStateQuery.h"

bool UWorldStateQuery::Evaluate_Implementation(const AAgent* Agent, const AAgentAIController* Controller, const UDecisionComponent* DecisionComponent) const
{
	(void)Agent;
	(void)Controller;
	(void)DecisionComponent;
	return false;
}
