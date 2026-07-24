// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ThrowGrenade.h
 * @brief Declares the GOAP action that activates the grenade throw ability.
 */

#pragma once

#include "CoreMinimal.h"
#include "Action.h"
#include "ThrowGrenade.generated.h"

class AAgentAIController;
struct FGrenadeThrowSolution;

/**
 * @brief Runtime action that passes a calculated grenade launch velocity to GA_ThrowGrenade.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UThrowGrenade : public UAction
{
	GENERATED_BODY()

public:
	/** @brief Initializes the grenade action's preconditions and effects. */
	UThrowGrenade();

	/**
	 * @brief Activates the owning agent's grenade throw ability with the current calculated trajectory.
	 *
	 * @param Planner Planner executing the grenade throw action.
	 * @return Running after ability activation, Failed when the throw cannot start, or Invalid without a planner.
	 */
	virtual EActionResult Execute(UPlanner* Planner) override;

protected:
	/** Retrieves the cached target location used by this grenade action variant. */
	virtual bool GetCachedTargetLocation(const AAgentAIController& Controller, FVector& OutTargetLocation) const;

	/** Retrieves the cached trajectory used by this grenade action variant. */
	virtual bool GetCachedThrowSolution(const AAgentAIController& Controller, FGrenadeThrowSolution& OutSolution) const;
};
