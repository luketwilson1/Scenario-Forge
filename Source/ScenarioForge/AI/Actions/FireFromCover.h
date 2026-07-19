// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FireFromCover.h
 * @brief Declares the action that fires at a visible target while occupying cover.
 */

#pragma once

#include "CoreMinimal.h"
#include "FireWeapon.h"
#include "FireFromCover.generated.h"

/**
 * @brief Fires the agent's primary weapon when the agent is peeking from cover and sees an enemy.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UFireFromCover : public UFireWeapon
{
	GENERATED_BODY()

public:
	/** @brief Adds State.InCover and State.Cover.Peeking to the inherited FireWeapon preconditions. */
	UFireFromCover();
};
