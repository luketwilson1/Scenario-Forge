// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GE_Damage.h
 * @brief Declares the instant Gameplay Effect used to subtract agent health.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Damage.generated.h"

/**
 * @brief Instant gameplay effect that applies SetByCaller damage to an agent's Health attribute.
 */
UCLASS()
class SCENARIOFORGE_API UGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:

	/** Configures the effect to subtract SetByCaller Data.Damage from Health instantly. */
	UGE_Damage();
};
