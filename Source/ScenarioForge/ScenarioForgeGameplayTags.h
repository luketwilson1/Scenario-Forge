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

/** State indicating the current visible target is within the equipped weapon's firing range. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_InWeaponRange);

/** State indicating the current visible target is within the agent's melee range. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_InMeleeRange);

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

/** Decision state indicating the agent remembers an enemy that is no longer visible. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_RemembersEnemy);

/** Decision state indicating the agent has destroyed its current target. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_DestroyTarget);

/** Decision state indicating the owning agent is dead. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead);

/** State indicating the agent is incapacitated until its health rises above its configured threshold. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Downed);

/** Decision state indicating the agent is near an active danger source. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Danger);

/** Decision state indicating an active grenade is close enough to threaten the agent. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_GrenadeNear);

/** Decision state indicating a hostile bullet recently passed near the agent. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_FiredUpon);

/** Decision state indicating the agent is currently dodging. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dodging);

/** Decision state indicating the agent has completed a self-preservation action. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_SelfPreserve);

/** Decision state indicating the agent has reached a claimed cover location. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_InCover);

/** Decision state indicating the agent has no known threat history. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Idle);

/** Decision state indicating the agent is aware of a threat but has no current visual engagement. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Alert);

/** Decision state indicating the agent or its squad currently has visual enemy contact. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Engage);

/** Decision state indicating the agent has a grenade and is not on grenade cooldown. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_CanThrowGrenade);

/** Decision state indicating a valid throwable enemy cluster is currently cached. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_TargetsGrouped);

/** Decision state indicating at least one stationary enemy is a valid grenade target. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_StationaryTarget);
