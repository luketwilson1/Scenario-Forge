// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Goal.h
 * @brief Declares a selectable GOAP goal.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Goal.generated.h"

/**
 * @brief Describes when a goal is eligible and which world states it wants satisfied.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class SCENARIOFORGE_API UGoal : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief Calculates this goal's current importance for the supplied world state.
	 *
	 * The default implementation returns Score while the goal is unsatisfied and zero once it is satisfied.
	 * Override this in a goal subclass or Blueprint to calculate context-sensitive utility.
	 *
	 * @param CurrentStates Current true world-state tags.
	 * @return Utility used by the Reasoner to compare unsatisfied goals.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Goal")
	float GetUtility(const FGameplayTagContainer& CurrentStates) const;

	/** State tags that should be true when this goal is satisfied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal")
	FGameplayTagContainer TrueStates;

	/** State tags that should be false when this goal is satisfied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal")
	FGameplayTagContainer FalseStates;

	/** Runtime importance assigned from the owning agent's Agent Sheet. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal|Selection", meta = (DisplayName = "Score"))
	float Score = 0.0f;
};
