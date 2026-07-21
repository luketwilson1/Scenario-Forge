// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Melee.h
 * @brief Declares the GOAP action that activates the agent's melee ability.
 */

#pragma once

#include "CoreMinimal.h"
#include "Action.h"
#include "Melee.generated.h"

/** Activates GA_Melee when the current visible target is within melee range. */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UMelee : public UAction
{
	GENERATED_BODY()

public:
	/** Configures melee planning preconditions and effects. */
	UMelee();

	/** Activates the granted melee gameplay ability. */
	virtual EActionResult Execute(UPlanner* Planner) override;
};
