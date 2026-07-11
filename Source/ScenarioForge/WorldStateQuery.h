// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file WorldStateQuery.h
 * @brief Declares reusable runtime queries that resolve GOAP state tags.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldStateQuery.generated.h"

class AAgent;
class AAgentAIController;
class UDecisionComponent;

/**
 * @brief Base class for runtime checks that answer whether a GOAP state tag is currently true.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCENARIOFORGE_API UWorldStateQuery : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief Evaluates this query against an agent and its decision context.
	 *
	 * @param Agent Agent pawn being evaluated.
	 * @param Controller AI controller that owns the decision component.
	 * @param DecisionComponent Decision component requesting the query.
	 * @return True when the queried world state is currently true.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "World State Query")
	bool Evaluate(const AAgent* Agent, const AAgentAIController* Controller, const UDecisionComponent* DecisionComponent) const;
	virtual bool Evaluate_Implementation(const AAgent* Agent, const AAgentAIController* Controller, const UDecisionComponent* DecisionComponent) const;
};
