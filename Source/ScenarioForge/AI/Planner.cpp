// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Planner.cpp
 * @brief Implements GOAP state tracking and A* action planning.
 */

#include "Planner.h"

#include "Actions/Action.h"
#include "Agent.h"
#include "AgentAIController.h"
#include "AgentCustomization.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "ScenarioForgeGameplayTags.h"
#include "Reasoner.h"
#include "WorldStateQuery.h"

/**
 * @brief Initializes the component and builds its first action plan.
 *
 * The plan is generated from the configured current state, goal state, and
 * available actions when gameplay begins.
 */
void UPlanner::BeginPlay()
{
	Super::BeginPlay();

	if (bIsDecisionMakingShutdown)
	{
		return;
	}

	RefreshGoalFromReasoner();
	RebuildCurrentPlan();
}

/**
 * @brief Replaces the available action subclasses and rebuilds the active plan.
 *
 * @param NewActions Action subclasses and planning costs available to this planner.
 */
void UPlanner::SetActions(const TMap<TSubclassOf<UAction>, float>& NewActions)
{
	if (bIsDecisionMakingShutdown)
	{
		return;
	}

	Actions = NewActions;
	RefreshGoalFromReasoner();
	RebuildCurrentPlan();
}

/**
 * @brief Replaces the planner goal states and rebuilds the current plan.
 *
 * @param NewTrueGoalStates Goal tags that should be present.
 * @param NewFalseGoalStates Goal tags that should be absent.
 */
void UPlanner::SetGoalStates(const FGameplayTagContainer& NewTrueGoalStates, const FGameplayTagContainer& NewFalseGoalStates)
{
	if (bIsDecisionMakingShutdown)
	{
		return;
	}

	TrueGoalStates = NewTrueGoalStates;
	FalseGoalStates = NewFalseGoalStates;
	RebuildCurrentPlan();
}

/**
 * @brief Adds a world-state tag and rebuilds the active plan when the state changes.
 *
 * @param StateTag State tag to add to the current GOAP state.
 */
void UPlanner::AddCurrentState(const FGameplayTag& StateTag)
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
	DrawStateChangeDebug(StateTag, true);
	RefreshGoalFromReasoner();
	ReplanAfterStateChange();
}

/**
 * @brief Removes a world-state tag and rebuilds the active plan when the state changes.
 *
 * @param StateTag State tag to remove from the current GOAP state.
 */
void UPlanner::RemoveCurrentState(const FGameplayTag& StateTag)
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
	DrawStateChangeDebug(StateTag, false);
	RefreshGoalFromReasoner();
	ReplanAfterStateChange();
}

void UPlanner::DrawStateChangeDebug(const FGameplayTag& StateTag, const bool bAdded) const
{
	const AAgentAIController* Controller = Cast<AAgentAIController>(GetOwner());
	const AAgent* Agent = Controller ? Cast<AAgent>(Controller->GetPawn()) : nullptr;
	const UAgentCustomization* AgentCustomization = Agent ? Agent->GetAgentCustomization() : nullptr;
	if (!GEngine || !AgentCustomization || !AgentCustomization->GetResolvedDrawStateChangeDebug())
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(
		-1,
		4.0f,
		bAdded ? FColor::Green : FColor::Red,
		FString::Printf(
			TEXT("Planner [%s]: %s %s"),
			*GetNameSafe(Agent),
			bAdded ? TEXT("Added") : TEXT("Removed"),
			*StateTag.ToString()));
}

bool UPlanner::EvaluateStateTag(const FGameplayTag& StateTag) const
{
	if (!StateTag.IsValid())
	{
		return false;
	}

	const AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetOwner());
	const AAgent* Agent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	const UAgentCustomization* AgentCustomization = Agent ? Agent->GetAgentCustomization() : nullptr;
	if (!AgentAIController || !Agent || !AgentCustomization)
	{
		return false;
	}

	const TSubclassOf<UWorldStateQuery>* QueryClass = AgentCustomization->GetResolvedStateQueries().Find(StateTag);
	if (!QueryClass || !*QueryClass)
	{
		return false;
	}

	const UWorldStateQuery* Query = NewObject<UWorldStateQuery>(const_cast<UPlanner*>(this), *QueryClass);
	return Query && Query->Evaluate(Agent, AgentAIController, this);
}

bool UPlanner::RefreshStateTagFromQuery(const FGameplayTag& StateTag)
{
	if (bIsDecisionMakingShutdown || !StateTag.IsValid())
	{
		return false;
	}

	const bool bStateIsTrue = EvaluateStateTag(StateTag);
	const bool bCurrentlyHasState = CurrentStates.HasTagExact(StateTag);
	if (bStateIsTrue && !bCurrentlyHasState)
	{
		AddCurrentState(StateTag);
	}
	else if (!bStateIsTrue && bCurrentlyHasState)
	{
		RemoveCurrentState(StateTag);
	}

	return bStateIsTrue;
}

/**
 * @brief Clears all planner data and leaves only a terminal current-state tag.
 *
 * @param TerminalStateTag Final tag that explains why decision making stopped.
 */
void UPlanner::ShutdownDecisionMaking(const FGameplayTag& TerminalStateTag)
{
	bIsDecisionMakingShutdown = true;
	bIsActionRunning = false;
	ActiveAction = nullptr;

	Actions.Reset();
	TrueGoalStates.Reset();
	FalseGoalStates.Reset();
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
void UPlanner::RebuildCurrentPlan()
{
	if (bIsDecisionMakingShutdown || bIsActionRunning)
	{
		return;
	}

	CurrentPlan = BuildPlan();
	ExecuteCurrentPlan();
}

/**
 * @brief Rebuilds against the latest state and safely preempts a more expensive active action.
 *
 * Actions that cannot guarantee interruption cleanup continue to completion. A
 * newly valid action only preempts when it is the new plan's first step and has
 * a strictly lower configured cost than the active action.
 */
void UPlanner::ReplanAfterStateChange()
{
	if (bIsDecisionMakingShutdown)
	{
		return;
	}

	if (bIsExecutingPlan)
	{
		bReplanRequested = true;
		return;
	}

	if (!bIsActionRunning || !ActiveAction)
	{
		RebuildCurrentPlan();
		return;
	}

	TArray<TSubclassOf<UAction>> NewPlan = BuildPlan();
	if (NewPlan.IsEmpty() || !NewPlan[0] || ActiveAction->IsA(NewPlan[0]))
	{
		return;
	}

	const float NewActionCost = FMath::Max(0.0f, Actions.FindRef(NewPlan[0]));
	const float ActiveActionCost = FMath::Max(0.0f, ActiveAction->ActionCost);
	if (NewActionCost >= ActiveActionCost || !ActiveAction->Interrupt(this))
	{
		return;
	}

	bIsActionRunning = false;
	ActiveAction = nullptr;
	CurrentPlan = MoveTemp(NewPlan);
	ExecuteCurrentPlan();
}

/**
 * @brief Releases an asynchronous action lock when its behavior reaches a terminal result.
 *
 * @param Result Terminal action result. Running preserves the current lock.
 */
void UPlanner::CompleteActiveAction(EActionResult Result)
{
	if (bIsDecisionMakingShutdown || !bIsActionRunning || Result == EActionResult::Running)
	{
		return;
	}

	bIsActionRunning = false;
	ActiveAction = nullptr;
	RebuildCurrentPlan();
}

void UPlanner::RefreshGoalFromReasoner()
{
	const AAgentAIController* Controller = Cast<AAgentAIController>(GetOwner());
	UReasoner* Reasoner = Controller ? Controller->GetReasoner() : nullptr;
	if (Reasoner)
	{
		/** Planner state mutations call this method, so the reasoner re-evaluates utility after every state change. */
		Reasoner->ChooseGoal(CurrentStates);
		TrueGoalStates = Reasoner->SelectedTrueStates;
		FalseGoalStates = Reasoner->SelectedFalseStates;
	}
}

/**
 * @brief Executes the first action in the current plan by instancing its class.
 */
void UPlanner::ExecuteCurrentPlan()
{
	if (bIsDecisionMakingShutdown || bIsExecutingPlan || bIsActionRunning || CurrentPlan.IsEmpty())
	{
		return;
	}

	/** Use the first action in the plan as the next immediate action to execute. */
	const TSubclassOf<UAction> ActionClass = CurrentPlan[0];
	if (!ActionClass)
	{
		return;
	}

	/** Spawn a transient instance of the selected action subclass. */
	ActiveAction = NewObject<UAction>(this, ActionClass);
	if (!ActiveAction)
	{
		return;
	}
	ActiveAction->ActionCost = Actions.FindRef(ActionClass);

	/** Lock before execution so synchronous state changes cannot launch a second action during startup. */
	bIsActionRunning = true;
	bIsExecutingPlan = true;
	const EActionResult ActionResult = ActiveAction->Execute(this);
	bIsExecutingPlan = false;

	if (ActionResult != EActionResult::Running)
	{
		bIsActionRunning = false;
		ActiveAction = nullptr;
	}

	if (bReplanRequested)
	{
		bReplanRequested = false;
		ReplanAfterStateChange();
	}
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
 * @return An ordered array of action subclasses from the current state to the goal state,
 *         or an empty array when no valid plan can be found.
 */
TArray<TSubclassOf<UAction>> UPlanner::BuildPlan()
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

		/** Action subclass used to transition from the parent to this node. */
		TSubclassOf<UAction> Action;

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
	const auto CalculateHeuristic = [](const FGameplayTagContainer& State, const FGameplayTagContainer& TrueGoal, const FGameplayTagContainer& FalseGoal)
	{
		int32 UnsatisfiedGoals = 0;
		TArray<FGameplayTag> GoalTags;
		TrueGoal.GetGameplayTagArray(GoalTags);

		for (const FGameplayTag& GoalTag : GoalTags)
		{
			if (!State.HasTagExact(GoalTag))
			{
				++UnsatisfiedGoals;
			}
		}

		GoalTags.Reset();
		FalseGoal.GetGameplayTagArray(GoalTags);
		for (const FGameplayTag& GoalTag : GoalTags)
		{
			if (State.HasTagExact(GoalTag))
			{
				++UnsatisfiedGoals;
			}
		}

		return static_cast<float>(UnsatisfiedGoals);
	};

	/** Candidate nodes awaiting exploration. */
	TArray<TSharedPtr<FDecisionPlanNode>> Open;

	/** Lowest known path cost for each previously encountered state. */
	TMap<FString, float> BestCosts;

	FGameplayTagContainer PlannerCurrentStates = CurrentStates;
	if (const AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetOwner()))
	{
		if (const AAgent* Agent = Cast<AAgent>(AgentAIController->GetPawn()))
		{
			if (const UAbilitySystemComponent* AbilitySystemComponent = Agent->GetAbilitySystemComponent())
			{
				FGameplayTagContainer OwnedAbilityTags;
				AbilitySystemComponent->GetOwnedGameplayTags(OwnedAbilityTags);
				PlannerCurrentStates.AppendTags(OwnedAbilityTags);
			}
		}
	}

	// Seed the search with world state plus runtime ability-system state tags.
	const TSharedPtr<FDecisionPlanNode> StartNode = MakeShared<FDecisionPlanNode>();
	StartNode->State = PlannerCurrentStates;
	StartNode->GCost = 0.0f;
	StartNode->HCost = CalculateHeuristic(PlannerCurrentStates, TrueGoalStates, FalseGoalStates);

	Open.Add(StartNode);
	BestCosts.Add(MakeStateKey(PlannerCurrentStates), 0.0f);

	while (!Open.IsEmpty())
	{
		// Explore the candidate with the lowest estimated total cost first.
		Open.Sort([](const TSharedPtr<FDecisionPlanNode>& A, const TSharedPtr<FDecisionPlanNode>& B)
		{
			return A->GetFCost() < B->GetFCost();
		});

		const TSharedPtr<FDecisionPlanNode> Current = Open[0];
		Open.RemoveAt(0);

		if (Current->State.HasAllExact(TrueGoalStates) && !Current->State.HasAnyExact(FalseGoalStates))
		{
			// Follow parent links back to the start and restore forward action order.
			TArray<TSubclassOf<UAction>> Plan;
			TSharedPtr<FDecisionPlanNode> PlanNode = Current;

			while (PlanNode.IsValid() && PlanNode->Action)
			{
				Plan.Insert(PlanNode->Action, 0);
				PlanNode = PlanNode->Parent;
			}

			return Plan;
		}

		for (const TPair<TSubclassOf<UAction>, float>& ActionEntry : Actions)
		{
			const TSubclassOf<UAction>& ActionClass = ActionEntry.Key;
			const UAction* Action = ActionClass ? ActionClass->GetDefaultObject<UAction>() : nullptr;
			if (!Action
				|| !Current->State.HasAllExact(Action->TruePreconditions)
				|| Current->State.HasAnyExact(Action->FalsePreconditions))
			{
				continue;
			}

			FGameplayTagContainer NextState = Current->State;

			// Apply the action's effects to create the successor state.
			NextState.RemoveTags(Action->RemovedEffects);
			NextState.AppendTags(Action->AddedEffects);

			const float NextCost = Current->GCost + FMath::Max(0.0f, ActionEntry.Value);
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
			NextNode->Action = ActionClass;
			NextNode->GCost = NextCost;
			NextNode->HCost = CalculateHeuristic(NextState, TrueGoalStates, FalseGoalStates);

			BestCosts.Add(NextStateKey, NextCost);
			Open.Add(NextNode);
		}
	}

	// The search exhausted every reachable state without satisfying the goal.
	return {};
}
