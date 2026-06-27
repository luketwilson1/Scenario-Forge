// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GA_FireWeapon.h
 * @brief Declares the gameplay ability used to fire a weapon.
 */

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_FireWeapon.generated.h"

/**
 * @brief Gameplay ability responsible for firing an equipped weapon.
 */
UCLASS(Abstract, Blueprintable)
class SCENARIOFORGE_API UGA_FireWeapon : public UGameplayAbility
{
	GENERATED_BODY()
};
