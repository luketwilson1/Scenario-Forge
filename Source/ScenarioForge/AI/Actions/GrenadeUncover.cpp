// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GrenadeUncover.cpp
 * @brief Implements the stationary-enemy grenade flush action.
 */

#include "GrenadeUncover.h"

#include "AgentAIController.h"
#include "ScenarioForgeGameplayTags.h"

UGrenadeUncover::UGrenadeUncover()
{
	TruePreconditions.Reset();
	TruePreconditions.AddTag(TAG_State_StationaryTarget.GetTag());
	TruePreconditions.AddTag(TAG_State_CanThrowGrenade.GetTag());
}

bool UGrenadeUncover::GetCachedTargetLocation(const AAgentAIController& Controller, FVector& OutTargetLocation) const
{
	return Controller.GetStationaryGrenadeTargetLocation(OutTargetLocation);
}

bool UGrenadeUncover::GetCachedThrowSolution(const AAgentAIController& Controller, FGrenadeThrowSolution& OutSolution) const
{
	return Controller.GetStationaryGrenadeThrowSolution(OutSolution);
}
