// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FireFromCover.cpp
 * @brief Implements the action that fires at a visible target from cover.
 */

#include "FireFromCover.h"

#include "ScenarioForgeGameplayTags.h"

/**
 * @brief Requires cover in addition to FireWeapon's visible-target precondition.
 */
UFireFromCover::UFireFromCover()
{
	TruePreconditions.AddTag(TAG_State_InCover.GetTag());
}
