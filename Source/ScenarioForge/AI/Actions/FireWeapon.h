// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FireWeapon.h
 * @brief Declares the GOAP action that fires the agent's equipped weapon.
 */

#pragma once

#include "CoreMinimal.h"
#include "Action.h"
#include "FireWeapon.generated.h"

/**
 * @brief Runtime action that fires the owning agent's primary weapon at the current enemy target.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UFireWeapon : public UAction
{
	GENERATED_BODY()

public:
	/** @brief Initializes the fire action's preconditions and effects. */
	UFireWeapon();

	/**
	 * @brief Fires the possessed agent's primary weapon at the current visible enemy target.
	 *
	 * @param Planner Planner executing the fire weapon action.
	 * @return Succeeded after firing, Failed when firing requirements are missing, or Invalid without a planner.
	 */
	virtual EActionResult Execute(UPlanner* Planner) override;
};
