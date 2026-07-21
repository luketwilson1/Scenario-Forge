// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AI/CoverTypes.h
 * @brief Declares shared cover authoring types.
 */

#pragma once

#include "CoreMinimal.h"
#include "CoverTypes.generated.h"

/** Describes the ways an agent can expose itself from an authored cover position. */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECoverType : uint8
{
	None = 0 UMETA(Hidden),
	Crouch = 1 << 0 UMETA(DisplayName = "Crouch"),
	Left = 1 << 1 UMETA(DisplayName = "Left"),
	Right = 1 << 2 UMETA(DisplayName = "Right")
};

ENUM_CLASS_FLAGS(ECoverType);
