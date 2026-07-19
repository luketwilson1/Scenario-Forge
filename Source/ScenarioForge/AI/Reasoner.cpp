// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Reasoner.cpp
 * @brief Implements goal selection for an AI agent.
 */

#include "Reasoner.h"

#include "Agent.h"
#include "AgentAIController.h"
#include "AgentCustomization.h"
#include "Engine/Engine.h"
#include "Goal.h"

/**
 * @brief Replaces the goal classes evaluated by this reasoner.
 *
 * @param NewGoalScores Resolved goal subclasses and their importance scores.
 */
void UReasoner::Configure(const TMap<TSubclassOf<UGoal>, float>& NewGoalScores)
{
	Goals.Reset();
	for (const TPair<TSubclassOf<UGoal>, float>& GoalEntry : NewGoalScores)
	{
		if (GoalEntry.Key)
		{
			if (UGoal* Goal = NewObject<UGoal>(this, GoalEntry.Key))
			{
				Goal->Score = GoalEntry.Value;
				Goals.Add(Goal);
			}
		}
	}
}

/**
 * @brief Chooses the highest-utility unsatisfied goal for the supplied state.
 *
 * @param CurrentStates Current true world-state tags.
 * @return True when the selected goal's true or false desired states changed.
 */
bool UReasoner::ChooseGoal(const FGameplayTagContainer& CurrentStates)
{
	UGoal* PreviousSelectedGoal = SelectedGoal;
	UGoal* NewSelectedGoal = nullptr;
	FGameplayTagContainer NewSelectedTrueStates;
	FGameplayTagContainer NewSelectedFalseStates;
	FName NewSelectedGoalName = NAME_None;
	float BestUtility = 0.0f;

	for (UGoal* Goal : Goals)
	{
		if (!Goal || (Goal->TrueStates.IsEmpty() && Goal->FalseStates.IsEmpty()))
		{
			continue;
		}

		/** A goal is satisfied when every true state is present and every false state is absent. */
		const bool bGoalSatisfied = CurrentStates.HasAllExact(Goal->TrueStates)
			&& !CurrentStates.HasAnyExact(Goal->FalseStates);
		const float GoalUtility = Goal->GetUtility(CurrentStates);
		if (bGoalSatisfied || GoalUtility <= BestUtility)
		{
			continue;
		}

		NewSelectedGoal = Goal;
		NewSelectedTrueStates = Goal->TrueStates;
		NewSelectedFalseStates = Goal->FalseStates;
		NewSelectedGoalName = Goal->GetFName();
		BestUtility = GoalUtility;
	}

	const float NewSelectedGoalUtility = NewSelectedGoalName != NAME_None ? BestUtility : 0.0f;
	const bool bGoalStatesChanged = !SelectedTrueStates.HasAllExact(NewSelectedTrueStates)
		|| !NewSelectedTrueStates.HasAllExact(SelectedTrueStates)
		|| !SelectedFalseStates.HasAllExact(NewSelectedFalseStates)
		|| !NewSelectedFalseStates.HasAllExact(SelectedFalseStates);

	SelectedGoal = NewSelectedGoal;
	SelectedTrueStates = NewSelectedTrueStates;
	SelectedFalseStates = NewSelectedFalseStates;
	SelectedGoalName = NewSelectedGoalName;
	SelectedGoalUtility = NewSelectedGoalUtility;

	if (PreviousSelectedGoal != NewSelectedGoal)
	{
		const AAgentAIController* Controller = Cast<AAgentAIController>(GetOwner());
		const AAgent* Agent = Controller ? Cast<AAgent>(Controller->GetPawn()) : nullptr;
		const UAgentCustomization* AgentCustomization = Agent ? Agent->GetAgentCustomization() : nullptr;
		if (GEngine && AgentCustomization && AgentCustomization->GetResolvedDrawGoalChangeDebug())
		{
			const FString PreviousGoalName = PreviousSelectedGoal
				? PreviousSelectedGoal->GetClass()->GetName()
				: TEXT("None");
			const FString NewGoalName = NewSelectedGoal
				? NewSelectedGoal->GetClass()->GetName()
				: TEXT("None");

			GEngine->AddOnScreenDebugMessage(
				-1,
				4.0f,
				FColor::Cyan,
				FString::Printf(
					TEXT("Reasoner [%s]: %s -> %s (Utility %.2f)"),
					*GetNameSafe(Agent),
					*PreviousGoalName,
					*NewGoalName,
					SelectedGoalUtility));
		}
	}

	return bGoalStatesChanged;
}
