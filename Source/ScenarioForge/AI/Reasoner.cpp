// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Reasoner.cpp
 * @brief Implements goal selection for an AI agent.
 */

#include "Reasoner.h"

#include "Goal.h"

/**
 * @brief Replaces the goal objects evaluated by this reasoner.
 *
 * @param NewGoals Resolved inherited and local goal objects.
 */
void UReasoner::Configure(const TArray<TObjectPtr<UGoal>>& NewGoals)
{
	Goals = NewGoals;
}

/**
 * @brief Chooses the highest-scoring unsatisfied goal for the supplied state.
 *
 * @param CurrentStates Current true world-state tags.
 * @return True when the selected goal's true or false desired states changed.
 */
bool UReasoner::ChooseGoal(const FGameplayTagContainer& CurrentStates)
{
	UGoal* NewSelectedGoal = nullptr;
	FGameplayTagContainer NewSelectedTrueStates;
	FGameplayTagContainer NewSelectedFalseStates;
	FName NewSelectedGoalName = NAME_None;
	int32 BestScore = TNumericLimits<int32>::Min();

	for (UGoal* Goal : Goals)
	{
		if (!Goal || (Goal->TrueStates.IsEmpty() && Goal->FalseStates.IsEmpty()))
		{
			continue;
		}

		/** A goal is satisfied when every true state is present and every false state is absent. */
		const bool bGoalSatisfied = CurrentStates.HasAllExact(Goal->TrueStates)
			&& !CurrentStates.HasAnyExact(Goal->FalseStates);
		if (bGoalSatisfied || Goal->Score <= BestScore)
		{
			continue;
		}

		NewSelectedGoal = Goal;
		NewSelectedTrueStates = Goal->TrueStates;
		NewSelectedFalseStates = Goal->FalseStates;
		NewSelectedGoalName = Goal->GetFName();
		BestScore = Goal->Score;
	}

	const int32 NewSelectedGoalScore = NewSelectedGoalName != NAME_None ? BestScore : 0;
	const bool bGoalStatesChanged = !SelectedTrueStates.HasAllExact(NewSelectedTrueStates)
		|| !NewSelectedTrueStates.HasAllExact(SelectedTrueStates)
		|| !SelectedFalseStates.HasAllExact(NewSelectedFalseStates)
		|| !NewSelectedFalseStates.HasAllExact(SelectedFalseStates);

	SelectedGoal = NewSelectedGoal;
	SelectedTrueStates = NewSelectedTrueStates;
	SelectedFalseStates = NewSelectedFalseStates;
	SelectedGoalName = NewSelectedGoalName;
	SelectedGoalScore = NewSelectedGoalScore;
	return bGoalStatesChanged;
}
