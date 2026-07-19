// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ScenarioForgeGameplayTags.cpp
 * @brief Defines native gameplay tags used by Scenario Forge systems.
 */

#include "ScenarioForgeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Damage, "Data.Damage", "SetByCaller value used by damage gameplay effects.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Weapon_BurstSeparation, "State.Weapon.BurstSeparation", "The agent is pausing between weapon bursts.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cooldown_AI_ThrowGrenade, "Cooldown.AI.ThrowGrenade", "The AI agent cannot currently choose another grenade throw action.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cooldown_AI_DodgeDanger, "Cooldown.AI.DodgeDanger", "The AI agent cannot currently choose another danger dodge action.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Equipment_Grenade_Frag, "Equipment.Grenade.Frag", "Equipment identifier for frag grenades.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Animation_ThrowGrenade_Release, "Event.Animation.ThrowGrenade.Release", "Animation event fired at the grenade throw release frame.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cover_Stance_Standing, "Cover.Stance.Standing", "The cover slot supports a standing stance.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cover_Peek_Left, "Cover.Peek.Left", "The cover slot supports peeking from the left side.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cover_Peek_Right, "Cover.Peek.Right", "The cover slot supports peeking from the right side.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cover_Peek_Up, "Cover.Peek.Up", "The cover slot makes an arriving agent crouch, then stand after its configured delay.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Idle, "State.Idle", "The agent is currently idle.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_SeesEnemy, "State.SeesEnemy", "The agent currently has visual contact with at least one enemy.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_DestroyTarget, "State.DestroyTarget", "The agent has destroyed its current target.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Dead, "State.Dead", "The agent is dead and should not perform normal behaviors.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Danger, "State.Danger", "The agent is near an active danger source.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_GrenadeNear, "State.GrenadeNear", "An active grenade is close enough to threaten the agent.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_FiredUpon, "State.FiredUpon", "A hostile bullet recently passed near the agent.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Dodging, "State.Dodging", "The agent is currently dodging danger.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_SelfPreserve, "State.SelfPreserve", "The agent has completed a self-preservation action.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_InCover, "State.InCover", "The agent has reached a cover location.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Combat_Idle, "State.Combat.Idle", "The agent has no known threat history.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Combat_Alert, "State.Combat.Alert", "The agent is aware of a threat but has no current visual engagement.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Combat_Engage, "State.Combat.Engage", "The agent or its squad currently has visual enemy contact.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Grenade_CanThrow, "State.Grenade.CanThrow", "All grenade planning checks currently pass.");
