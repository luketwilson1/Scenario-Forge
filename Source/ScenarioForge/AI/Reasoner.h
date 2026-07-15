// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Reasoner.h
 * @brief Declares the component that chooses an agent's active goal.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Reasoner.generated.h"

class UGoal;

/**
 * @brief Chooses the highest-scoring unsatisfied goal for the owning agent.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class SCENARIOFORGE_API UReasoner : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * @brief Replaces the selectable goal objects.
	 *
	 * @param NewGoals Resolved inherited and local goals available to this agent.
	 */
	void Configure(const TArray<TObjectPtr<UGoal>>& NewGoals);

	/**
	 * @brief Re-evaluates all goals against the supplied current state.
	 *
	 * @param CurrentStates Current true world-state tags.
	 * @return True when the selected goal's desired states changed.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Reasoner")
	bool ChooseGoal(const FGameplayTagContainer& CurrentStates);

	/** First-class goal assets available to this reasoner. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Reasoner")
	TArray<TObjectPtr<UGoal>> Goals;

	/** Goal asset currently selected, or null when every authored goal is satisfied. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Reasoner")
	TObjectPtr<UGoal> SelectedGoal;

	/** World states the selected goal wants to be true. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Reasoner")
	FGameplayTagContainer SelectedTrueStates;

	/** World states the selected goal wants to be false. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Reasoner")
	FGameplayTagContainer SelectedFalseStates;

	/** Name of the selected goal, or None when no goal is selected. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Reasoner")
	FName SelectedGoalName = NAME_None;

	/** Score of the selected goal. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Reasoner")
	int32 SelectedGoalScore = 0;

};
