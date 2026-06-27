// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file IdleActionBehavior.h
 * @brief Declares the GOAP action behavior that marks an agent idle.
 */

#pragma once

#include "CoreMinimal.h"
#include "ActionBehavior.h"
#include "IdleActionBehavior.generated.h"

/**
 * @brief Runtime behavior for an idle action.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UIdleActionBehavior : public UActionBehavior
{
	GENERATED_BODY()

public:

	/**
	 * @brief Adds State.Idle to the executing agent's decision state.
	 *
	 * @param Agent Decision component executing the idle behavior.
	 * @param ActionDefinition Action definition that selected this behavior.
	 */
	virtual void Execute_Implementation(UDecisionComponent* Agent, const UActionDefinition* ActionDefinition) override;
};
