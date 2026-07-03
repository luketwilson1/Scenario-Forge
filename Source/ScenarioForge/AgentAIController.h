// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentAIController.h
 * @brief Declares the AI controller that owns agent decisions and perception.
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionTypes.h"
#include "AgentAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UAgentCustomization;
class UDecisionComponent;
class UTacticalPositioningComponent;

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
	 * @brief Gets the component that owns tactical movement intent.
	 *
	 * @return Tactical positioning component used by this controller.
	 */
	UTacticalPositioningComponent* GetTacticalPositioningComponent() const;

	/**
	 * @brief Gets the first valid enemy actor currently visible to this controller.
	 *
	 * @return Current visible enemy target, or nullptr when no enemy is visible.
	 */
	AActor* GetCurrentEnemyTarget() const;

	/** Re-evaluates tactical movement mode from perception and cover-condition state. */
	void RefreshTacticalMovementMode();

protected:

	/**
	 * @brief Pulls agent customization from the possessed pawn and applies controller settings.
	 *
	 * @param InPawn Pawn newly possessed by this controller.
	 */
	virtual void OnPossess(APawn* InPawn) override;

	/** Applies action and perception configuration from the possessed agent customization asset. */
	void ApplyAgentCustomization();

	/** Mirrors relevant ability-system state tags into the decision component. */
	void BindAbilitySystemStateTags(APawn* InPawn);

	/**
	 * @brief Checks whether the supplied actor is an enemy of this controller's agent.
	 *
	 * @param Actor Actor to classify.
	 * @return True when the actor is an agent with a different faction.
	 */
	bool IsEnemyActor(const AActor* Actor) const;

	/** Updates the State.SeesEnemy decision tag from the current visible enemy list. */
	void RefreshSeesEnemyState();

	/** Returns whether cover movement should be preferred over normal combat movement. */
	bool ShouldUseCoverMovement() const;

	/** Disables orient-to-movement and focuses the current enemy during combat movement. */
	void ApplyCombatRotationSettings(AActor* FocusTarget);

	/** Restores pawn rotation settings saved before entering combat movement. */
	void RestoreNonCombatRotationSettings();

	/**
	 * @brief Handles perception updates and maintains the visible enemy list.
	 *
	 * @param Actor Actor whose perception state changed.
	 * @param Stimulus Perception stimulus data reported by Unreal AI perception.
	 */
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/**
	 * @brief Mirrors burst separation tag count changes into GOAP current state.
	 *
	 * @param Tag Tag whose active count changed.
	 * @param NewCount New active count for the tag.
	 */
	void HandleBurstSeparationTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** Data asset used to configure this agent's actions, perception, and presentation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent")
	TObjectPtr<UAgentCustomization> AgentCustomization;

	/** GOAP planner and plan state owned by this controller. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UDecisionComponent> DecisionComponent;

	/** Tactical position selection and movement intent owned by this controller. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UTacticalPositioningComponent> TacticalPositioningComponent;

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

	/** Saved pawn bUseControllerRotationYaw value from before combat rotation took over. */
	bool bSavedUseControllerRotationYaw = false;

	/** Saved movement bOrientRotationToMovement value from before combat rotation took over. */
	bool bSavedOrientRotationToMovement = false;

	/** True once pre-combat rotation settings have been captured. */
	bool bHasSavedRotationSettings = false;

	/** True once this controller has perceived at least one enemy during the current possession. */
	bool bHasSeenEnemy = false;
};
