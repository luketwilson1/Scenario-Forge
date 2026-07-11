// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GrenadeTypes.h
 * @brief Declares grenade type identifiers used by gameplay and data assets.
 */

#pragma once

#include "CoreMinimal.h"
#include "GrenadeTypes.generated.h"

class UEquipmentCustomization;

/**
 * @brief Grenade types available to agents and gameplay systems.
 */
UENUM(BlueprintType)
enum class EGrenadeType : uint8
{
	FragGrenade UMETA(DisplayName = "Frag Grenade"),
	None UMETA(DisplayName = "None")
};

/**
 * @brief Ordered starting grenade entry. The first valid entry becomes the initially equipped grenade.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FStartingGrenade
{
	GENERATED_BODY()

public:
	/** Equipment sheet used to spawn this grenade. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	TObjectPtr<UEquipmentCustomization> Equipment;

	/** Number of grenades assigned for this type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade", meta = (ClampMin = "0", UIMin = "0"))
	int32 Count = 0;
};
