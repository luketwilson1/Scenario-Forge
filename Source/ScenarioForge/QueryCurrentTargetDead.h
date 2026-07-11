// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file QueryCurrentTargetDead.h
 * @brief Declares the world-state query that checks whether the current enemy target is dead.
 */

#pragma once

#include "CoreMinimal.h"
#include "WorldStateQuery.h"
#include "QueryCurrentTargetDead.generated.h"

/**
 * @brief Resolves State.CurrentTargetDead from the controller's current enemy target.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Query Current Target Dead"))
class SCENARIOFORGE_API UQueryCurrentTargetDead : public UWorldStateQuery
{
	GENERATED_BODY()

public:
	virtual bool Evaluate_Implementation(const AAgent* Agent, const AAgentAIController* Controller, const UDecisionComponent* DecisionComponent) const override;
};
