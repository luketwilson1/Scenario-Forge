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
	 * @return Running while a timed burst fires, Succeeded after an instantaneous shot, Failed when firing requirements are missing, or Invalid without a planner.
	 */
	virtual EActionResult Execute(UPlanner* Planner) override;

	/**
	 * @brief Stops the active weapon burst so a cheaper newly valid action may run.
	 *
	 * @param Planner Planner requesting interruption.
	 * @return True when the owning agent's active weapon burst was stopped.
	 */
	virtual bool Interrupt(UPlanner* Planner) override;
};
