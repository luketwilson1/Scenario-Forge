// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file PawnCustomization.h
 * @brief Declares shared pawn presentation data used by agents and playable characters.
 */

#pragma once

#include "CoreMinimal.h"
#include "CustomizationTypes.h"
#include "Engine/DataAsset.h"
#include "PawnCustomization.generated.h"

/**
 * @brief Stores reusable pawn appearance data such as skeletal mesh, animation blueprint, and material overrides.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API UPawnCustomization : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Visual presentation used by the pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FAppearance Appearance;
};
