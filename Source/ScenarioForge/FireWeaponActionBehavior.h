// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FireWeaponActionBehavior.h
 * @brief Declares the GOAP action behavior that fires the agent's equipped weapon.
 */

#pragma once

#include "CoreMinimal.h"
#include "ActionBehavior.h"
#include "FireWeaponActionBehavior.generated.h"

/**
 * @brief Runtime behavior that fires the owning agent's primary weapon at the current enemy target.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UFireWeaponActionBehavior : public UActionBehavior
{
	GENERATED_BODY()

public:

	/**
	 * @brief Fires the possessed agent's primary weapon at the current visible enemy target.
	 *
	 * @param Agent Decision component executing the fire weapon behavior.
	 * @param ActionDefinition Action definition that selected this behavior.
	 */
	virtual void Execute_Implementation(UDecisionComponent* Agent, const UActionDefinition* ActionDefinition) override;
};
