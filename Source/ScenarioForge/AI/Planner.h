// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Planner.h
 * @brief Declares the GOAP planner used to plan agent actions.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Planner.generated.h"

class UAction;
enum class EActionResult : uint8;

/**
 * @brief Maintains current/goal state tags and builds action plans using GOAP search.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SCENARIOFORGE_API UPlanner : public UActorComponent
{
	GENERATED_BODY()

public:

	/**
	 * @brief Replaces the available action set and rebuilds the current plan.
	 *
	 * @param NewActions Action subclasses and planning costs available to the planner.
	 */
	void SetActions(const TMap<TSubclassOf<UAction>, float>& NewActions);

	/**
	 * @brief Replaces the goal state set and rebuilds the current plan.
	 *
	 * @param NewTrueGoalStates Goal tags that should be present.
	 * @param NewFalseGoalStates Goal tags that should be absent.
	 */
	void SetGoalStates(const FGameplayTagContainer& NewTrueGoalStates, const FGameplayTagContainer& NewFalseGoalStates);

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
	 * @brief Finishes the currently locked asynchronous action and resumes planning.
	 *
	 * @param Result Terminal result reported by the active action. Running does not release the lock.
	 */
	void CompleteActiveAction(EActionResult Result);

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

	/** Action subclasses and planning costs currently available for planning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TMap<TSubclassOf<UAction>, float> Actions;

	/** Gameplay tags describing the agent's current planning state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FGameplayTagContainer CurrentStates;

	/** Gameplay tags the planner is trying to make true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FGameplayTagContainer TrueGoalStates;

	/** Gameplay tags the planner is trying to make false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FGameplayTagContainer FalseGoalStates;

	/** Most recently built plan from current state to goal state. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TArray<TSubclassOf<UAction>> CurrentPlan;

protected:
	/** Builds the initial plan when gameplay begins. */
	virtual void BeginPlay() override;

private:
	/** Prints an on-screen message for a current-state mutation when enabled by the Agent Sheet. */
	void DrawStateChangeDebug(const FGameplayTag& StateTag, bool bAdded) const;

	/**
	 * @brief Rebuilds the current plan and immediately executes its first action when available.
	 */
	void RebuildCurrentPlan();

	/**
	 * @brief Replans after a world-state mutation and preempts a running action when the new first action is cheaper.
	 */
	void ReplanAfterStateChange();

	/** Copies the Reasoner's selected desired states into this planner. */
	void RefreshGoalFromReasoner();

	/**
	 * @brief Executes the first action in the current plan.
	 */
	void ExecuteCurrentPlan();

	/**
	 * @brief Builds a GOAP action sequence from current state to goal state.
	 *
	 * @return Ordered action subclasses that satisfy the goal, or empty when no plan is found.
	 */
	TArray<TSubclassOf<UAction>> BuildPlan();

	/** True once the planner has been shut down by a terminal state such as death. */
	bool bIsDecisionMakingShutdown = false;

	/** True while an action behavior is executing to prevent recursive plan execution. */
	bool bIsExecutingPlan = false;

	/** True while an asynchronous action has reported Running and has not yet reported a terminal result. */
	bool bIsActionRunning = false;

	/** True when a state mutation requested replanning during synchronous action startup. */
	bool bReplanRequested = false;

	/** Runtime instance of the most recently executed action. */
	UPROPERTY(Transient)
	TObjectPtr<UAction> ActiveAction;
};
