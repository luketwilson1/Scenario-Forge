// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ScenarioForgeGameplayTags.h
 * @brief Declares native gameplay tags used by Scenario Forge systems.
 */

#pragma once

#include "NativeGameplayTags.h"

/** Runtime magnitude tag used to pass damage into GE_Damage. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage);

/** State indicating the agent is pausing between weapon bursts. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Weapon_BurstSeparation);

/** Decision state indicating the agent is idle. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Idle);

/** Decision state indicating the agent currently sees at least one enemy. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_SeesEnemy);

/** Decision state indicating the agent's current target has died. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_CurrentTargetDead);

/** Decision state indicating the owning agent is dead. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead);
