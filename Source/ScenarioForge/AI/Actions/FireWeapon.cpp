// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FireWeapon.cpp
 * @brief Implements the GOAP action that fires an equipped weapon at the current enemy target.
 */

#include "FireWeapon.h"

#include "AgentAIController.h"
#include "Planner.h"
#include "ScenarioForgeGameplayTags.h"
#include "WeaponFiringComponent.h"

/**
 * @brief Configures the fire action's planning preconditions and effects.
 */
UFireWeapon::UFireWeapon()
{
	bCanBeInterrupted = true;
	TruePreconditions.AddTag(TAG_State_SeesEnemy.GetTag());
	TruePreconditions.AddTag(TAG_State_InWeaponRange.GetTag());
	FalsePreconditions.AddTag(TAG_State_Dead.GetTag());
	FalsePreconditions.AddTag(TAG_State_Weapon_BurstSeparation.GetTag());
	AddedEffects.AddTag(TAG_State_DestroyTarget.GetTag());
}

/**
 * @brief Fires the owner agent's primary weapon at the controller's current enemy target.
 *
 * @param Planner Planner executing the fire weapon action.
 * @return Succeeded after firing, Failed when firing requirements are missing, or Invalid without a planner.
 */
EActionResult UFireWeapon::Execute(UPlanner* Planner)
{
	if (!Planner)
	{
		return EActionResult::Invalid;
	}

	AAgentAIController* AgentAIController = Cast<AAgentAIController>(Planner->GetOwner());
	UWeaponFiringComponent* WeaponChannel = AgentAIController
		? AgentAIController->GetWeaponFiringComponent()
		: nullptr;
	return WeaponChannel && WeaponChannel->BeginDeliberateFire(Planner)
		? EActionResult::Running
		: EActionResult::Failed;
}

/**
 * @brief Stops the owning agent's active burst before the planner replaces this action.
 *
 * @param Planner Planner requesting interruption.
 * @return True when an equipped primary weapon was found and stopped.
 */
bool UFireWeapon::Interrupt(UPlanner* Planner)
{
	AAgentAIController* AgentAIController = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	UWeaponFiringComponent* WeaponChannel = AgentAIController
		? AgentAIController->GetWeaponFiringComponent()
		: nullptr;
	return WeaponChannel && WeaponChannel->CancelDeliberateFire(Planner);
}
