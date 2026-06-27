// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ScenarioForgeGameplayTags.cpp
 * @brief Defines native gameplay tags used by Scenario Forge systems.
 */

#include "ScenarioForgeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Damage, "Data.Damage", "SetByCaller value used by damage gameplay effects.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Idle, "State.Idle", "The agent is currently idle.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_SeesEnemy, "State.SeesEnemy", "The agent currently has visual contact with at least one enemy.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_CurrentTargetDead, "State.CurrentTargetDead", "The agent's current target is dead.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Dead, "State.Dead", "The agent is dead and should not perform normal behaviors.");
