// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Zone.h
 * @brief Declares authored tactical zones that group areas.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Zone.generated.h"

class AArea;

/**
 * @brief Designer-authored tactical zone containing related areas.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API AZone : public AActor
{
	GENERATED_BODY()

public:
	/** Initializes this tactical zone actor. */
	AZone();

	/** Designer-facing zone label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Zone")
	FString ZoneName;

	/** Areas grouped under this tactical zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Zone", meta = (NoElementDuplicate))
	TArray<TObjectPtr<AArea>> Areas;
};
