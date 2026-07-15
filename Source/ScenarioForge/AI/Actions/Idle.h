// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Idle.h
 * @brief Declares the GOAP action that marks an agent idle.
 */

#pragma once

#include "CoreMinimal.h"
#include "Action.h"
#include "Idle.generated.h"

/**
 * @brief Runtime action for an idle action.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UIdle : public UAction
{
	GENERATED_BODY()

public:
	/** @brief Initializes the idle action's preconditions. */
	UIdle();

	/**
	 * @brief Adds State.Idle to the executing planner state.
	 *
	 * @param Planner Planner executing the idle action.
	 * @return Succeeded after entering idle, or Invalid without a planner.
	 */
	virtual EActionResult Execute(UPlanner* Planner) override;
};
