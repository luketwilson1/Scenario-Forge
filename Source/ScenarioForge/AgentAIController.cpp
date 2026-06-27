// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentAIController.cpp
 * @brief Implements AI perception setup and decision-state synchronization.
 */

#include "AgentAIController.h"

#include "Agent.h"
#include "AgentCustomization.h"
#include "DecisionComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "ScenarioForgeGameplayTags.h"

/**
 * @brief Creates the decision and perception components used by agent AI.
 */
AAgentAIController::AAgentAIController()
{
	DecisionComponent = CreateDefaultSubobject<UDecisionComponent>(TEXT("DecisionComponent"));

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->ConfigureSense(*HearingConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AAgentAIController::HandleTargetPerceptionUpdated);
}

/**
 * @brief Gets the decision component owned by this controller.
 *
 * @return Decision component used for GOAP planning.
 */
UDecisionComponent* AAgentAIController::GetDecisionComponent() const
{
	return DecisionComponent;
}

/**
 * @brief Gets the first valid visible enemy as a temporary combat target.
 *
 * @return First valid seen enemy, or nullptr when none are visible.
 */
AActor* AAgentAIController::GetCurrentEnemyTarget() const
{
	for (AActor* SeenEnemy : SeenEnemies)
	{
		if (IsValid(SeenEnemy))
		{
			return SeenEnemy;
		}
	}

	return nullptr;
}

/**
 * @brief Copies customization from the possessed agent and configures controller systems.
 *
 * @param InPawn Pawn newly possessed by this controller.
 */
void AAgentAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const AAgent* Agent = Cast<AAgent>(InPawn);
	AgentCustomization = Agent ? Agent->GetAgentCustomization() : nullptr;
	SeenEnemies.Reset();

	ApplyAgentCustomization();
	RefreshSeesEnemyState();
}

/**
 * @brief Determines whether an actor is an agent on a different faction.
 *
 * @param Actor Actor to test.
 * @return True when the actor is an opposing-faction agent.
 */
bool AAgentAIController::IsEnemyActor(const AActor* Actor) const
{
	const AAgent* OtherAgent = Cast<AAgent>(Actor);
	const UAgentCustomization* OtherCustomization = OtherAgent ? OtherAgent->GetAgentCustomization() : nullptr;

	return AgentCustomization
		&& OtherCustomization
		&& AgentCustomization->Faction != OtherCustomization->Faction;
}

/**
 * @brief Rebuilds the State.SeesEnemy decision tag from the current visible enemies.
 */
void AAgentAIController::RefreshSeesEnemyState()
{
	SeenEnemies.RemoveAll([](const TObjectPtr<AActor>& SeenEnemy)
	{
		return !IsValid(SeenEnemy);
	});

	if (!DecisionComponent)
	{
		return;
	}

	if (SeenEnemies.IsEmpty())
	{
		DecisionComponent->RemoveCurrentState(TAG_State_SeesEnemy.GetTag());
	}
	else
	{
		DecisionComponent->AddCurrentState(TAG_State_SeesEnemy.GetTag());
	}
}

/**
 * @brief Responds to sight perception changes and keeps the visible enemy list current.
 *
 * @param Actor Actor whose perception state changed.
 * @param Stimulus Perception stimulus reported by Unreal.
 */
void AAgentAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed() && IsEnemyActor(Actor))
	{
		const bool bAlreadySeen = SeenEnemies.ContainsByPredicate([Actor](const TObjectPtr<AActor>& SeenEnemy)
		{
			return SeenEnemy.Get() == Actor;
		});

		if (!bAlreadySeen)
		{
			SeenEnemies.Add(Actor);
		}
	}
	else
	{
		SeenEnemies.RemoveAll([Actor](const TObjectPtr<AActor>& SeenEnemy)
		{
			return SeenEnemy.Get() == Actor;
		});
	}

	RefreshSeesEnemyState();
}

/**
 * @brief Applies agent sheet values to decision actions and AI perception ranges.
 */
void AAgentAIController::ApplyAgentCustomization()
{
	if (!AgentCustomization)
	{
		return;
	}

	if (DecisionComponent)
	{
		/** Push data-asset actions and goals into the planner when this controller possesses an agent. */
		DecisionComponent->SetActions(AgentCustomization->Actions);
		DecisionComponent->SetGoalStates(AgentCustomization->StartingGoalTags);
	}

	if (SightConfig)
	{
		SightConfig->SightRadius = AgentCustomization->Perception.SightRadius;
		SightConfig->LoseSightRadius = AgentCustomization->Perception.LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = AgentCustomization->Perception.PeripheralVisionAngleDegrees;
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = AgentCustomization->Perception.HearingRange;
	}

	if (AIPerceptionComponent && SightConfig && HearingConfig)
	{
		AIPerceptionComponent->ConfigureSense(*SightConfig);
		AIPerceptionComponent->ConfigureSense(*HearingConfig);
		AIPerceptionComponent->RequestStimuliListenerUpdate();
	}
}
