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
class ASquad;

/**
 * @brief Runtime threat posture used to control AI evaluation frequency.
 */
UENUM(BlueprintType)
enum class EAgentCombatState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Alert UMETA(DisplayName = "Alert"),
	Engage UMETA(DisplayName = "Engage")
};

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

	/**
	 * @brief Checks whether this controller currently has sight contact with an enemy actor.
	 *
	 * @param EnemyActor Enemy actor to search for in this controller's visible enemy list.
	 * @return True when the actor is currently visible to this controller.
	 */
	bool IsSeeingEnemyActor(const AActor* EnemyActor) const;

	/**
	 * @brief Checks whether this controller currently remembers a no-longer-visible enemy actor.
	 *
	 * @param EnemyActor Enemy actor to search for in this controller's remembered enemy list.
	 * @return True when the actor is currently remembered by this controller.
	 */
	bool IsRememberingEnemyActor(const AActor* EnemyActor) const;

	/**
	 * @brief Gets the most recent grenade target location selected by grenade eligibility evaluation.
	 *
	 * @param OutTargetLocation Receives the selected grenade target location when available.
	 * @return True when a valid grenade target location is currently cached.
	 */
	bool GetCurrentGrenadeTargetLocation(FVector& OutTargetLocation) const;

	/** Re-evaluates tactical movement mode from perception and cover-condition state. */
	void RefreshTacticalMovementMode();

protected:

	/**
	 * @brief Pulls agent customization from the possessed pawn and applies controller settings.
	 *
	 * @param InPawn Pawn newly possessed by this controller.
	 */
	virtual void OnPossess(APawn* InPawn) override;

	/** Clears squad perception contributions before this controller releases its pawn. */
	virtual void OnUnPossess() override;

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

	/** Checks whether an enemy actor is close enough to remain remembered after sight is lost. */
	bool IsEnemyWithinRememberRadius(const AActor* EnemyActor) const;

	/** Updates the State.SeesEnemy decision tag from the current visible enemy list. */
	void RefreshSeesEnemyState();

	/** Updates runtime combat state from personal and squad perception knowledge. */
	void RefreshCombatState();

	/** Assigns runtime combat state and updates state-dependent evaluators. */
	void SetCombatState(EAgentCombatState NewCombatState);

	/** Mirrors the runtime combat state into GOAP current-state tags. */
	void RefreshCombatStateTags();

	/** Starts the combat-only grenade condition evaluation timer. */
	void StartGrenadeEvalTimer();

	/** Stops the combat-only grenade condition evaluation timer. */
	void StopGrenadeEvalTimer();

	/** Gets the configured grenade evaluation interval, or a safe fallback when unset. */
	float GetGrenadeEvalInterval() const;

	/** Updates grenade-related GOAP state from enemy clustering, range, cooldown, and collateral checks. */
	void RefreshGrenadeDecisionState();

	/** Finds the best seen-or-known enemy cluster center for a grenade throw. */
	bool FindBestGrenadeClusterCenter(FVector& OutClusterCenter, int32& OutClusterEnemyCount) const;

	/** Checks whether the configured grenade throw velocity can reach a target point. */
	bool CanReachGrenadeTarget(const FVector& TargetLocation) const;

	/** Checks whether a grenade target point would include friendly agents inside the collateral radius. */
	bool HasFriendlyInGrenadeCollateralRadius(const FVector& TargetLocation) const;

	/** Finds the authored squad that contains this controller's possessed agent. */
	ASquad* FindOwningSquad() const;

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

	/**
	 * @brief Re-evaluates grenade GOAP state when the AI grenade cooldown tag changes.
	 *
	 * @param Tag Tag whose active count changed.
	 * @param NewCount New active count for the tag.
	 */
	void HandleGrenadeCooldownTagChanged(const FGameplayTag Tag, int32 NewCount);

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

	/** Enemy actors no longer visible but still close enough for this controller to remember. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TArray<TObjectPtr<AActor>> RememberedEnemies;

	/** Cached grenade target location selected by the most recent grenade decision refresh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Grenade")
	FVector CurrentGrenadeTargetLocation = FVector::ZeroVector;

	/** True when CurrentGrenadeTargetLocation contains a valid grenade target point. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Grenade")
	bool bHasCurrentGrenadeTargetLocation = false;

	/** Current runtime threat posture. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	EAgentCombatState CombatState = EAgentCombatState::Idle;

	/** Timer that refreshes grenade condition state while engaged. */
	FTimerHandle GrenadeEvalTimerHandle;

	/** Saved pawn bUseControllerRotationYaw value from before combat rotation took over. */
	bool bSavedUseControllerRotationYaw = false;

	/** True once pre-combat rotation settings have been captured. */
	bool bHasSavedRotationSettings = false;

	/** True once this controller has perceived at least one enemy during the current possession. */
	bool bHasSeenEnemy = false;
};
