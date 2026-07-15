// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file DodgeDanger.h
 * @brief Declares the GOAP action that dodges away from nearby danger.
 */

#pragma once

#include "CoreMinimal.h"
#include "Action.h"
#include "DodgeDanger.generated.h"

/**
 * @brief Selects a valid dodge direction and exposes dodge state to animation.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UDodgeDanger : public UAction
{
	GENERATED_BODY()

public:
	/** @brief Initializes the dodge action's preconditions and effects. */
	UDodgeDanger();

	/**
	 * @brief Starts a dodge request using the owning agent's resolved dodge properties.
	 *
	 * @param Planner Planner executing the dodge action.
	 * @return Running when the dodge is scheduled, Failed when it cannot start, or Invalid for a bad execution context.
	 */
	virtual EActionResult Execute(UPlanner* Planner) override;
};
