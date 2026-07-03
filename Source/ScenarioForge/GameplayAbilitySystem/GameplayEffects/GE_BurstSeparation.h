// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GE_BurstSeparation.h
 * @brief Declares the duration Gameplay Effect used to mark burst separation pauses.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_BurstSeparation.generated.h"

/**
 * @brief Duration effect whose spec grants State.Weapon.BurstSeparation while active.
 */
UCLASS()
class SCENARIOFORGE_API UGE_BurstSeparation : public UGameplayEffect
{
	GENERATED_BODY()

public:

	/** Configures the effect as a duration-only state holder. */
	UGE_BurstSeparation();
};
