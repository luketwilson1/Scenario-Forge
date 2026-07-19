// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Goal.cpp
 * @brief Implements default GOAP goal utility evaluation.
 */

#include "Goal.h"

/**
 * @brief Returns the authored base utility while this goal remains unsatisfied.
 *
 * @param CurrentStates Current true world-state tags.
 * @return Zero for a satisfied goal; otherwise its designer-authored base utility.
 */
float UGoal::GetUtility_Implementation(const FGameplayTagContainer& CurrentStates) const
{
	const bool bGoalSatisfied = CurrentStates.HasAllExact(TrueStates)
		&& !CurrentStates.HasAnyExact(FalseStates);
	return bGoalSatisfied ? 0.0f : Score;
}
