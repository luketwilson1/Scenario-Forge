// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Objective.h
 * @brief Declares authored objectives that reference a tactical zone.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Objective.generated.h"

class AZone;

/**
 * @brief Designer-authored objective associated with a tactical zone.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API AObjective : public AActor
{
	GENERATED_BODY()

public:
	/** Initializes this objective actor. */
	AObjective();

	/** Designer-facing objective label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Objective")
	FString ObjectiveName;

	/** Zone associated with this objective. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Objective")
	TObjectPtr<AZone> Zone;
};
