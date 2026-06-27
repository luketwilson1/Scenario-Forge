// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FactionTypes.h
 * @brief Declares faction identifiers shared by agents and future faction-owned actors.
 */

#pragma once

#include "CoreMinimal.h"
#include "FactionTypes.generated.h"

/**
 * @brief Identifies the faction an actor or asset belongs to.
 */
UENUM(BlueprintType)
enum class EFaction : uint8
{
	/** Red faction identifier. */
	Red UMETA(DisplayName = "Red"),

	/** Blue faction identifier. */
	Blue UMETA(DisplayName = "Blue")
};
