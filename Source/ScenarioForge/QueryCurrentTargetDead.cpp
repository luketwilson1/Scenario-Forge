// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file QueryCurrentTargetDead.cpp
 * @brief Implements current target death state resolution.
 */

#include "QueryCurrentTargetDead.h"

#include "AgentAIController.h"
#include "Agent.h"

bool UQueryCurrentTargetDead::Evaluate_Implementation(const AAgent* Agent, const AAgentAIController* Controller, const UDecisionComponent* DecisionComponent) const
{
	(void)Agent;
	(void)DecisionComponent;

	const AActor* TargetActor = Controller ? Controller->GetCurrentEnemyTarget() : nullptr;
	const AAgent* TargetAgent = Cast<AAgent>(TargetActor);
	return IsValid(TargetActor) && TargetAgent && TargetAgent->IsDead();
}
