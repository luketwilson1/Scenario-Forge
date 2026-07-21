// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AI/CoverActor.h
 * @brief Declares the native base actor used by authored cover Blueprints.
 */

#pragma once

#include "CoreMinimal.h"
#include "CoverTypes.h"
#include "GameFramework/Actor.h"
#include "CoverActor.generated.h"

class UArrowComponent;

/** Owns the runtime-readable cover mask and authored peek transforms. */
UCLASS(Blueprintable)
class SCENARIOFORGE_API ACoverActor : public AActor
{
	GENERATED_BODY()

public:
	/** Appends Blueprint-authored peek arrows enabled by CoverType. */
	void GetEnabledPeekPoints(TArray<UArrowComponent*>& OutPeekPoints) const;

	/** Supported cover exposures; multiple entries may be selected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover", meta = (Bitmask, BitmaskEnum = "/Script/ScenarioForge.ECoverType"))
	int32 CoverType = 0;
};
