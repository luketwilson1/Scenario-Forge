// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FireWeaponActionBehavior.cpp
 * @brief Implements the GOAP behavior that fires an equipped weapon at the current enemy target.
 */

#include "FireWeaponActionBehavior.h"

#include "Agent.h"
#include "AgentAIController.h"
#include "DecisionComponent.h"
#include "Weapon.h"

/**
 * @brief Fires the owner agent's primary weapon at the controller's current enemy target.
 *
 * @param Agent Decision component executing the fire weapon behavior.
 * @param ActionDefinition Action definition associated with this behavior.
 */
void UFireWeaponActionBehavior::Execute_Implementation(UDecisionComponent* Agent, const UActionDefinition* ActionDefinition)
{
	if (!Agent)
	{
		return;
	}

	/** Resolve the controller, pawn, target, and equipped weapon required to shoot. */
	AAgentAIController* AgentAIController = Cast<AAgentAIController>(Agent->GetOwner());
	AAgent* OwningAgent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	AActor* TargetActor = AgentAIController ? AgentAIController->GetCurrentEnemyTarget() : nullptr;
	AWeapon* PrimaryWeapon = OwningAgent ? OwningAgent->GetPrimaryWeapon() : nullptr;

	if (!PrimaryWeapon || !TargetActor)
	{
		return;
	}

	/** Aim at the target actor's current world location for this temporary behavior pass. */
	PrimaryWeapon->Fire(TargetActor->GetActorLocation());
}
