// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentAIController.cpp
 * @brief Implements AI perception setup and decision-state synchronization.
 */

#include "AgentAIController.h"

#include "AbilitySystemComponent.h"
#include "Agent.h"
#include "AgentCustomization.h"
#include "DecisionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySystem/AttributeSets/AgentAttributeSet.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "ScenarioForgeGameplayTags.h"
#include "TacticalPositioningComponent.h"

/**
 * @brief Creates the decision and perception components used by agent AI.
 */
AAgentAIController::AAgentAIController()
{
	DecisionComponent = CreateDefaultSubobject<UDecisionComponent>(TEXT("DecisionComponent"));
	TacticalPositioningComponent = CreateDefaultSubobject<UTacticalPositioningComponent>(TEXT("TacticalPositioningComponent"));

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
 * @brief Gets the tactical positioning component owned by this controller.
 *
 * @return Tactical positioning component used for movement intent and destination selection.
 */
UTacticalPositioningComponent* AAgentAIController::GetTacticalPositioningComponent() const
{
	return TacticalPositioningComponent;
}

/**
 * @brief Gets the first valid visible enemy as a temporary combat target.
 *
 * @return First valid seen enemy, or nullptr when none are visible.
 */
AActor* AAgentAIController::GetCurrentEnemyTarget() const
{
	// TODO: Replace this first-valid target fallback with evaluator-based target selection.
	// Evaluators should score visible enemies by criteria such as distance, threat, health, cover, and role.
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
	bHasSeenEnemy = false;

	ApplyAgentCustomization();
	BindAbilitySystemStateTags(InPawn);
	RefreshSeesEnemyState();
}

/**
 * @brief Subscribes to ability-system tags that should affect GOAP planning.
 *
 * @param InPawn Pawn whose ability system should be observed.
 */
void AAgentAIController::BindAbilitySystemStateTags(APawn* InPawn)
{
	AAgent* Agent = Cast<AAgent>(InPawn);
	UAbilitySystemComponent* AbilitySystemComponent = Agent ? Agent->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->RegisterGameplayTagEvent(
		TAG_State_Weapon_BurstSeparation.GetTag(),
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AAgentAIController::HandleBurstSeparationTagChanged);
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
		&& AgentCustomization->GetResolvedFaction() != OtherCustomization->GetResolvedFaction();
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

	if (!SeenEnemies.IsEmpty())
	{
		bHasSeenEnemy = true;
	}

	if (SeenEnemies.IsEmpty())
	{
		DecisionComponent->RemoveCurrentState(TAG_State_SeesEnemy.GetTag());
		RefreshTacticalMovementMode();
		RestoreNonCombatRotationSettings();
	}
	else
	{
		DecisionComponent->AddCurrentState(TAG_State_SeesEnemy.GetTag());
		AActor* CurrentEnemyTarget = GetCurrentEnemyTarget();
		RefreshTacticalMovementMode();
		ApplyCombatRotationSettings(CurrentEnemyTarget);
	}
}

/**
 * @brief Re-evaluates tactical movement mode from perception and cover-condition state.
 */
void AAgentAIController::RefreshTacticalMovementMode()
{
	if (!TacticalPositioningComponent)
	{
		return;
	}

	if (ShouldUseCoverMovement())
	{
		TacticalPositioningComponent->SetTargetActor(GetCurrentEnemyTarget());
		if (TacticalPositioningComponent->GetMovementMode() != ETacticalMovementMode::Uncover)
		{
			TacticalPositioningComponent->SetMovementMode(ETacticalMovementMode::Cover);
		}
		return;
	}

	if (!SeenEnemies.IsEmpty())
	{
		TacticalPositioningComponent->SetTargetActor(GetCurrentEnemyTarget());
		TacticalPositioningComponent->SetMovementMode(ETacticalMovementMode::Combat);
		return;
	}

	if (bHasSeenEnemy)
	{
		TacticalPositioningComponent->ClearTargetActor();
		TacticalPositioningComponent->SetMovementMode(ETacticalMovementMode::Search);
		return;
	}

	if (TacticalPositioningComponent->GetMovementMode() == ETacticalMovementMode::None)
	{
		TacticalPositioningComponent->ClearTargetActor();
		TacticalPositioningComponent->SetMovementMode(ETacticalMovementMode::Idle);
	}
}

/**
 * @brief Returns whether cover movement should be preferred over normal combat movement.
 */
bool AAgentAIController::ShouldUseCoverMovement() const
{
	const float CoverVitalityThreshold = AgentCustomization
		? FMath::Clamp(AgentCustomization->GetResolvedCoverProperties().CoverVitalityThreshold, 0.0f, 100.0f)
		: 0.0f;
	if (CoverVitalityThreshold <= 0.0f)
	{
		return false;
	}

	const AAgent* Agent = Cast<AAgent>(GetPawn());
	const UAbilitySystemComponent* AbilitySystemComponent = Agent ? Agent->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		return false;
	}

	const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(UAgentAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= 0.0f)
	{
		return false;
	}

	const float Health = AbilitySystemComponent->GetNumericAttribute(UAgentAttributeSet::GetHealthAttribute());
	const float HealthPercent = FMath::Clamp((Health / MaxHealth) * 100.0f, 0.0f, 100.0f);
	return HealthPercent <= CoverVitalityThreshold;
}

/**
 * @brief Makes combat target focus own yaw instead of movement direction.
 *
 * @param FocusTarget Enemy target to face while moving and shooting.
 */
void AAgentAIController::ApplyCombatRotationSettings(AActor* FocusTarget)
{
	AAgent* Agent = Cast<AAgent>(GetPawn());
	UCharacterMovementComponent* MovementComponent = Agent ? Agent->GetCharacterMovement() : nullptr;
	if (!Agent || !MovementComponent)
	{
		return;
	}

	if (!bHasSavedRotationSettings)
	{
		bSavedUseControllerRotationYaw = Agent->bUseControllerRotationYaw;
		bSavedOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
		bHasSavedRotationSettings = true;
	}

	MovementComponent->bOrientRotationToMovement = false;
	Agent->bUseControllerRotationYaw = true;

	if (FocusTarget)
	{
		SetFocus(FocusTarget, EAIFocusPriority::Gameplay);
	}
}

/**
 * @brief Restores normal movement-facing rotation after combat movement ends.
 */
void AAgentAIController::RestoreNonCombatRotationSettings()
{
	AAgent* Agent = Cast<AAgent>(GetPawn());
	UCharacterMovementComponent* MovementComponent = Agent ? Agent->GetCharacterMovement() : nullptr;
	if (Agent && MovementComponent && bHasSavedRotationSettings)
	{
		Agent->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
		MovementComponent->bOrientRotationToMovement = bSavedOrientRotationToMovement;
	}

	ClearFocus(EAIFocusPriority::Gameplay);
	bHasSavedRotationSettings = false;
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
 * @brief Mirrors burst separation gameplay effect state into the decision component.
 *
 * @param Tag Tag whose count changed.
 * @param NewCount New active count for the tag.
 */
void AAgentAIController::HandleBurstSeparationTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!DecisionComponent || !Tag.MatchesTagExact(TAG_State_Weapon_BurstSeparation.GetTag()))
	{
		return;
	}

	if (NewCount > 0)
	{
		DecisionComponent->AddCurrentState(TAG_State_Weapon_BurstSeparation.GetTag());
	}
	else
	{
		DecisionComponent->RemoveCurrentState(TAG_State_Weapon_BurstSeparation.GetTag());
	}
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
		DecisionComponent->SetActions(AgentCustomization->GetResolvedActions());
		DecisionComponent->SetGoalStates(AgentCustomization->GetResolvedStartingGoalTags());
	}

	const FPerception& ResolvedPerception = AgentCustomization->GetResolvedPerception();
	if (SightConfig)
	{
		SightConfig->SightRadius = ResolvedPerception.SightRadius;
		SightConfig->LoseSightRadius = ResolvedPerception.LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = ResolvedPerception.PeripheralVisionAngleDegrees;
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = ResolvedPerception.HearingRange;
	}

	if (AIPerceptionComponent && SightConfig && HearingConfig)
	{
		AIPerceptionComponent->ConfigureSense(*SightConfig);
		AIPerceptionComponent->ConfigureSense(*HearingConfig);
		AIPerceptionComponent->RequestStimuliListenerUpdate();
	}
}
