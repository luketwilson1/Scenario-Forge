// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ActionBehavior.h
 * @brief Declares the base runtime behavior object for GOAP action execution.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ActionBehavior.generated.h"

class UActionDefinition;
class UDecisionComponent;

/**
 * @brief Executes the runtime behavior for an action definition.
 */
UCLASS(Abstract, Blueprintable)
class SCENARIOFORGE_API UActionBehavior : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * @brief Executes an action selected by the planner.
	 *
	 * @param Agent Decision component executing the selected action.
	 * @param ActionDefinition Action data that selected this behavior.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Action")
	void Execute(UDecisionComponent* Agent, const UActionDefinition* ActionDefinition);
};
