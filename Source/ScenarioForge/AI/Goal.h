// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Goal.h
 * @brief Declares a selectable GOAP goal.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Goal.generated.h"

/**
 * @brief Describes when a goal is eligible and which world states it wants satisfied.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API UGoal : public UDataAsset
{
	GENERATED_BODY()

public:
	/** State tags that should be true when this goal is satisfied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal")
	FGameplayTagContainer TrueStates;

	/** State tags that should be false when this goal is satisfied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal")
	FGameplayTagContainer FalseStates;

	/** Higher-scoring eligible goals are preferred by the reasoner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal|Selection", meta = (DisplayName = "Score"))
	int32 Score = 0;
};
