// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GE_BurstSeparation.cpp
 * @brief Implements the burst separation Gameplay Effect.
 */

#include "GE_BurstSeparation.h"

/**
 * @brief Builds a duration effect used to carry dynamic granted cooldown tags.
 */
UGE_BurstSeparation::UGE_BurstSeparation()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
}
