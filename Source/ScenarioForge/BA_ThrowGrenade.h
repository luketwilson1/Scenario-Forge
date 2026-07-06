// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file BA_ThrowGrenade.h
 * @brief Declares the GOAP behavior action that activates the grenade throw ability.
 */

#pragma once

#include "CoreMinimal.h"
#include "ActionBehavior.h"
#include "BA_ThrowGrenade.generated.h"

/**
 * @brief Runtime behavior that passes a calculated grenade launch velocity to GA_ThrowGrenade.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UBA_ThrowGrenade : public UActionBehavior
{
	GENERATED_BODY()

public:
	/**
	 * @brief Activates the owning agent's grenade throw ability with the current calculated trajectory.
	 *
	 * @param Agent Decision component executing the grenade throw behavior.
	 * @param ActionDefinition Action definition that selected this behavior.
	 */
	virtual void Execute_Implementation(UDecisionComponent* Agent, const UActionDefinition* ActionDefinition) override;
};
