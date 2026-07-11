// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file DecisionComponent.h
 * @brief Declares the GOAP decision component used to plan agent actions.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DecisionComponent.generated.h"

class UActionDefinition;

/**
 * @brief Maintains current/goal state tags and builds action plans using GOAP search.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SCENARIOFORGE_API UDecisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/**
	 * @brief Replaces the available action set and rebuilds the current plan.
	 *
	 * @param NewActions Actions available to the planner.
	 */
	void SetActions(const TArray<TObjectPtr<UActionDefinition>>& NewActions);

	/**
	 * @brief Replaces the goal state set and rebuilds the current plan.
	 *
	 * @param NewGoalStates Goal tags the planner should satisfy.
	 */
	void SetGoalStates(const FGameplayTagContainer& NewGoalStates);

	/**
	 * @brief Adds a current state tag and rebuilds the plan when the state changes.
	 *
	 * @param StateTag Gameplay tag to add to current state.
	 */
	void AddCurrentState(const FGameplayTag& StateTag);

	/**
	 * @brief Removes a current state tag and rebuilds the plan when the state changes.
	 *
	 * @param StateTag Gameplay tag to remove from current state.
	 */
	void RemoveCurrentState(const FGameplayTag& StateTag);

	/**
	 * @brief Evaluates a state tag through the owning agent's resolved state-query map.
	 *
	 * @param StateTag Tag to evaluate.
	 * @return True when a mapped query reports the state is currently true.
	 */
	bool EvaluateStateTag(const FGameplayTag& StateTag) const;

	/**
	 * @brief Evaluates a state tag and mirrors the result into CurrentStates.
	 *
	 * @param StateTag Tag to refresh.
	 * @return True when a mapped query exists and reports the state is true.
	 */
	bool RefreshStateTagFromQuery(const FGameplayTag& StateTag);

	/**
	 * @brief Clears planning data, records a terminal state tag, and prevents future replanning.
	 *
	 * @param TerminalStateTag Final state tag to leave on the component after shutdown.
	 */
	void ShutdownDecisionMaking(const FGameplayTag& TerminalStateTag);

	/** Actions currently available for planning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TArray<TObjectPtr<UActionDefinition>> Actions;

	/** Gameplay tags describing the agent's current planning state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FGameplayTagContainer CurrentStates;

	/** Gameplay tags the planner is trying to satisfy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FGameplayTagContainer GoalStates;

	/** Fallback planner goals used when no state-driven goal rule matches. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	FGameplayTagContainer FallbackGoalStates;

	/** Name of the currently selected state-driven goal rule, or None when fallback goals are active. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	FName ActiveGoalRuleName = NAME_None;

	/** Score of the currently selected state-driven goal rule. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	int32 ActiveGoalRuleScore = 0;

	/** Most recently built plan from current state to goal state. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TArray<TObjectPtr<UActionDefinition>> CurrentPlan;

protected:
	/** Builds the initial plan when gameplay begins. */
	virtual void BeginPlay() override;

private:
	/**
	 * @brief Rebuilds the current plan and immediately executes its first action when available.
	 */
	void RebuildCurrentPlan();

	/**
	 * @brief Selects active goals from the owning agent's state-driven goal rules.
	 *
	 * @return True when GoalStates changed.
	 */
	bool SelectGoalStatesFromCurrentState();

	/**
	 * @brief Executes the first action in the current plan.
	 */
	void ExecuteCurrentPlan();

	/**
	 * @brief Builds a GOAP action sequence from current state to goal state.
	 *
	 * @return Ordered action definitions that satisfy the goal, or empty when no plan is found.
	 */
	TArray<TObjectPtr<UActionDefinition>> BuildPlan();

	/** True once the decision component has been shut down by a terminal state such as death. */
	bool bIsDecisionMakingShutdown = false;

	/** True while an action behavior is executing to prevent recursive plan execution. */
	bool bIsExecutingPlan = false;
};
