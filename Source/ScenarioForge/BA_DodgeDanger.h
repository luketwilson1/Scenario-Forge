// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file BA_DodgeDanger.h
 * @brief Declares the GOAP behavior that dodges away from nearby danger.
 */

#pragma once

#include "CoreMinimal.h"
#include "ActionBehavior.h"
#include "BA_DodgeDanger.generated.h"

/**
 * @brief Moves an agent away from its remembered danger source and exposes dodge state to animation.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UBA_DodgeDanger : public UActionBehavior
{
	GENERATED_BODY()

public:
	/** Starts a dodge move using the owning agent's resolved dodge properties. */
	virtual void Execute_Implementation(UDecisionComponent* Agent, const UActionDefinition* ActionDefinition) override;
};
