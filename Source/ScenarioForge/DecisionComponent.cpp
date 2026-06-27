// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file DecisionComponent.cpp
 * @brief Implements GOAP state tracking and A* action planning.
 */

#include "DecisionComponent.h"

#include "ActionDefinition.h"

/**
 * @brief Initializes the component and builds its first action plan.
 *
 * The plan is generated from the configured current state, goal state, and
 * available actions when gameplay begins.
 */
void UDecisionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bIsDecisionMakingShutdown)
	{
		return;
	}

	RebuildCurrentPlan();
}

/**
 * @brief Replaces the available action definitions and rebuilds the active plan.
 *
 * @param NewActions Action definitions available to this decision component.
 */
void UDecisionComponent::SetActions(const TArray<TObjectPtr<UActionDefinition>>& NewActions)
{
	if (bIsDecisionMakingShutdown)
	{
		return;
	}

	Actions = NewActions;
	RebuildCurrentPlan();
}

/**
 * @brief Replaces the planner goal states and rebuilds the current plan.
 *
 * @param NewGoalStates Goal tags the planner should satisfy.
 */
void UDecisionComponent::SetGoalStates(const FGameplayTagContainer& NewGoalStates)
{
	if (bIsDecisionMakingShutdown)
	{
		return;
	}

	GoalStates = NewGoalStates;
	RebuildCurrentPlan();
}

/**
 * @brief Adds a world-state tag and rebuilds the active plan when the state changes.
 *
 * @param StateTag State tag to add to the current GOAP state.
 */
void UDecisionComponent::AddCurrentState(const FGameplayTag& StateTag)
{
	if (bIsDecisionMakingShutdown)
	{
		return;
	}

	if (!StateTag.IsValid() || CurrentStates.HasTagExact(StateTag))
	{
		return;
	}

	CurrentStates.AddTag(StateTag);
	RebuildCurrentPlan();
}

/**
 * @brief Removes a world-state tag and rebuilds the active plan when the state changes.
 *
 * @param StateTag State tag to remove from the current GOAP state.
 */
void UDecisionComponent::RemoveCurrentState(const FGameplayTag& StateTag)
{
	if (bIsDecisionMakingShutdown)
	{
		return;
	}

	if (!StateTag.IsValid() || !CurrentStates.HasTagExact(StateTag))
	{
		return;
	}

	CurrentStates.RemoveTag(StateTag);
	RebuildCurrentPlan();
}

/**
 * @brief Clears all planner data and leaves only a terminal current-state tag.
 *
 * @param TerminalStateTag Final tag that explains why decision making stopped.
 */
void UDecisionComponent::ShutdownDecisionMaking(const FGameplayTag& TerminalStateTag)
{
	bIsDecisionMakingShutdown = true;

	Actions.Reset();
	GoalStates.Reset();
	CurrentPlan.Reset();
	CurrentStates.Reset();

	if (TerminalStateTag.IsValid())
	{
		CurrentStates.AddTag(TerminalStateTag);
	}
}

/**
 * @brief Rebuilds the current plan and executes the first available planned action.
 */
void UDecisionComponent::RebuildCurrentPlan()
{
	if (bIsDecisionMakingShutdown)
	{
		return;
	}

	CurrentPlan = BuildPlan();
	ExecuteCurrentPlan();
}

/**
 * @brief Executes the first action in the current plan by instancing its behavior class.
 */
void UDecisionComponent::ExecuteCurrentPlan()
{
	if (bIsDecisionMakingShutdown || bIsExecutingPlan || CurrentPlan.IsEmpty())
	{
		return;
	}

	/** Use the first action in the plan as the next immediate behavior to execute. */
	UActionDefinition* ActionDefinition = CurrentPlan[0];
	if (!ActionDefinition || !ActionDefinition->BehaviorClass)
	{
		return;
	}

	/** Spawn a transient behavior instance so action definitions stay data-only. */
	UActionBehavior* ActionBehavior = NewObject<UActionBehavior>(this, ActionDefinition->BehaviorClass);
	if (!ActionBehavior)
	{
		return;
	}

	bIsExecutingPlan = true;
	ActionBehavior->Execute(this, ActionDefinition);
	bIsExecutingPlan = false;
}

/**
 * @brief Builds the lowest-cost sequence of actions that satisfies the goal state.
 *
 * Uses an A* search in which each node represents a set of gameplay tags. An
 * action may expand a node when all of its required states are present. Its
 * effects are then applied to produce the next state.
 *
 * The accumulated action count is used as the path cost, while the number of
 * missing goal tags is used as the heuristic estimate.
 *
 * @return An ordered array of action definitions from the current state to the goal state,
 *         or an empty array when no valid plan can be found.
 */
TArray<TObjectPtr<UActionDefinition>> UDecisionComponent::BuildPlan()
{
	if (bIsDecisionMakingShutdown)
	{
		return {};
	}

	/**
	 * @brief A state explored by the A* planning search.
	 */
	struct FDecisionPlanNode
	{
		/** Gameplay tags describing the world state at this node. */
		FGameplayTagContainer State;

		/** Previous node in the candidate plan. */
		TSharedPtr<FDecisionPlanNode> Parent;

		/** Action definition used to transition from the parent to this node. */
		TObjectPtr<UActionDefinition> Action = nullptr;

		/** Number of actions taken from the initial state. */
		float GCost = 0.0f;

		/** Estimated number of unsatisfied goal tags. */
		float HCost = 0.0f;

		/**
		 * @brief Gets the estimated total cost of reaching the goal through this node.
		 *
		 * @return The sum of the accumulated path cost and heuristic cost.
		 */
		float GetFCost() const
		{
			return GCost + HCost;
		}
	};

	/**
	 * Produces a deterministic key for a gameplay-tag state.
	 *
	 * Tags are sorted before serialization so containers with identical tags
	 * generate the same key regardless of their internal ordering.
	 */
	const auto MakeStateKey = [](const FGameplayTagContainer& State)
	{
		TArray<FGameplayTag> Tags;
		State.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
		{
			return A.GetTagName().LexicalLess(B.GetTagName());
		});

		FString Key;
		for (const FGameplayTag& Tag : Tags)
		{
			Key += Tag.ToString();
			Key += TEXT("|");
		}

		return Key;
	};

	/**
	 * Estimates the remaining distance to the goal by counting goal tags that
	 * are not present in the supplied state.
	 */
	const auto CalculateHeuristic = [](const FGameplayTagContainer& State, const FGameplayTagContainer& Goal)
	{
		int32 MissingGoals = 0;
		TArray<FGameplayTag> GoalTags;
		Goal.GetGameplayTagArray(GoalTags);

		for (const FGameplayTag& GoalTag : GoalTags)
		{
			if (!State.HasTagExact(GoalTag))
			{
				++MissingGoals;
			}
		}

		return static_cast<float>(MissingGoals);
	};

	/** Candidate nodes awaiting exploration. */
	TArray<TSharedPtr<FDecisionPlanNode>> Open;

	/** Lowest known path cost for each previously encountered state. */
	TMap<FString, float> BestCosts;

	// Seed the search with the component's current world state.
	const TSharedPtr<FDecisionPlanNode> StartNode = MakeShared<FDecisionPlanNode>();
	StartNode->State = CurrentStates;
	StartNode->GCost = 0.0f;
	StartNode->HCost = CalculateHeuristic(CurrentStates, GoalStates);

	Open.Add(StartNode);
	BestCosts.Add(MakeStateKey(CurrentStates), 0.0f);

	while (!Open.IsEmpty())
	{
		// Explore the candidate with the lowest estimated total cost first.
		Open.Sort([](const TSharedPtr<FDecisionPlanNode>& A, const TSharedPtr<FDecisionPlanNode>& B)
		{
			return A->GetFCost() < B->GetFCost();
		});

		const TSharedPtr<FDecisionPlanNode> Current = Open[0];
		Open.RemoveAt(0);

		if (Current->State.HasAllExact(GoalStates))
		{
			// Follow parent links back to the start and restore forward action order.
			TArray<TObjectPtr<UActionDefinition>> Plan;
			TSharedPtr<FDecisionPlanNode> PlanNode = Current;

			while (PlanNode.IsValid() && PlanNode->Action)
			{
				Plan.Insert(PlanNode->Action, 0);
				PlanNode = PlanNode->Parent;
			}

			return Plan;
		}

		for (UActionDefinition* Action : Actions)
		{
			if (!Action
				|| !Current->State.HasAllExact(Action->RequiredTags)
				|| Current->State.HasAnyExact(Action->BlockedTags))
			{
				continue;
			}

			FGameplayTagContainer NextState = Current->State;

			// Apply the action's effects to create the successor state.
			NextState.RemoveTags(Action->RemovedTags);
			NextState.AppendTags(Action->AddedTags);

			const float NextCost = Current->GCost + 1.0f;
			const FString NextStateKey = MakeStateKey(NextState);

			if (const float* KnownCost = BestCosts.Find(NextStateKey))
			{
				if (*KnownCost <= NextCost)
				{
					// A cheaper or equal route to this state has already been found.
					continue;
				}
			}

			const TSharedPtr<FDecisionPlanNode> NextNode = MakeShared<FDecisionPlanNode>();
			NextNode->State = NextState;
			NextNode->Parent = Current;
			NextNode->Action = Action;
			NextNode->GCost = NextCost;
			NextNode->HCost = CalculateHeuristic(NextState, GoalStates);

			BestCosts.Add(NextStateKey, NextCost);
			Open.Add(NextNode);
		}
	}

	// The search exhausted every reachable state without satisfying the goal.
	return {};
}
