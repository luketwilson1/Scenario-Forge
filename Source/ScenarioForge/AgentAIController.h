// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentAIController.h
 * @brief Declares the AI controller that owns agent decisions and perception.
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "AgentAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UAgentCustomization;
class UDecisionComponent;

/**
 * @brief Coordinates AI perception, decisions, and agent customization.
 */
UCLASS()
class SCENARIOFORGE_API AAgentAIController : public AAIController
{
	GENERATED_BODY()

public:

	/** Initializes the decision component and AI perception senses. */
	AAgentAIController();

	/**
	 * @brief Gets the GOAP decision component owned by this controller.
	 *
	 * @return Decision component used by this controller.
	 */
	UDecisionComponent* GetDecisionComponent() const;

	/**
	 * @brief Gets the first valid enemy actor currently visible to this controller.
	 *
	 * @return Current visible enemy target, or nullptr when no enemy is visible.
	 */
	AActor* GetCurrentEnemyTarget() const;

protected:

	/**
	 * @brief Pulls agent customization from the possessed pawn and applies controller settings.
	 *
	 * @param InPawn Pawn newly possessed by this controller.
	 */
	virtual void OnPossess(APawn* InPawn) override;

	/** Applies action and perception configuration from the possessed agent customization asset. */
	void ApplyAgentCustomization();

	/**
	 * @brief Checks whether the supplied actor is an enemy of this controller's agent.
	 *
	 * @param Actor Actor to classify.
	 * @return True when the actor is an agent with a different faction.
	 */
	bool IsEnemyActor(const AActor* Actor) const;

	/** Updates the State.SeesEnemy decision tag from the current visible enemy list. */
	void RefreshSeesEnemyState();

	/**
	 * @brief Handles perception updates and maintains the visible enemy list.
	 *
	 * @param Actor Actor whose perception state changed.
	 * @param Stimulus Perception stimulus data reported by Unreal AI perception.
	 */
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** Data asset used to configure this agent's actions, perception, and presentation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent")
	TObjectPtr<UAgentCustomization> AgentCustomization;

	/** GOAP planner and plan state owned by this controller. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UDecisionComponent> DecisionComponent;

	/** Perception component configured from the agent customization. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	/** Sight sense configuration configured from the agent customization. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** Hearing sense configuration configured from the agent customization. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	/** Enemy actors currently visible to this controller. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TArray<TObjectPtr<AActor>> SeenEnemies;
};
