// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GrenadeTypes.h
 * @brief Declares grenade type identifiers used by gameplay and data assets.
 */

#pragma once

#include "CoreMinimal.h"
#include "GrenadeTypes.generated.h"

/**
 * @brief Grenade types available to agents and gameplay systems.
 */
UENUM(BlueprintType)
enum class EGrenadeType : uint8
{
	FragGrenade UMETA(DisplayName = "Frag Grenade")
};
