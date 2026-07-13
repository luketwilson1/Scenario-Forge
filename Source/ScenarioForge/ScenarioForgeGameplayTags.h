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

/** State indicating the agent is on AI grenade throw cooldown. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_AI_ThrowGrenade);

/** State indicating the agent is on AI danger dodge cooldown. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_AI_DodgeDanger);

/** Equipment identifier for frag grenades. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Equipment_Grenade_Frag);

/** Animation event fired when a grenade throw montage reaches the release frame. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Animation_ThrowGrenade_Release);

/** Decision state indicating the agent is idle. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Idle);

/** Decision state indicating the agent currently sees at least one enemy. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_SeesEnemy);

/** Decision state indicating the agent's current target has died. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_CurrentTargetDead);

/** Decision state indicating the owning agent is dead. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead);

/** Decision state indicating the agent is near an active danger source. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Danger);

/** Decision state indicating the agent is inside grenade danger range. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Danger_Grenade);

/** Decision state indicating the agent is currently dodging. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dodging);

/** Decision state indicating the agent has escaped its current danger source. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_SafeFromDanger);

/** Decision state indicating the agent has no known threat history. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Idle);

/** Decision state indicating the agent is aware of a threat but has no current visual engagement. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Alert);

/** Decision state indicating the agent or its squad currently has visual enemy contact. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Engage);

/** Decision state indicating all grenade planning checks currently pass. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Grenade_CanThrow);
