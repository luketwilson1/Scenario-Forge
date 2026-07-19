// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentAIController.h
 * @brief Declares the AI controller that owns agent decisions and perception.
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "GrenadeThrowFunctionLibrary.h"
#include "Perception/AIPerceptionTypes.h"
#include "SmartObjectRuntime.h"
#include "AgentAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UAgentCustomization;
class UPlanner;
class UReasoner;
class USmartObjectSubsystem;

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
 * @brief Direction in which an agent is actively leaning from its current cover slot.
 */
UENUM(BlueprintType)
enum class ECoverLeanDirection : uint8
{
	None UMETA(DisplayName = "None"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right")
};

/**
 * @brief Coordinates AI perception, decisions, and agent customization.
 */
UCLASS()
class SCENARIOFORGE_API AAgentAIController : public AAIController
{
	GENERATED_BODY()

public:

	/** Initializes the planner and AI perception senses. */
	AAgentAIController();

	/**
	 * @brief Gets the GOAP planner owned by this controller.
	 *
	 * @return Decision component used by this controller.
	 */
	UPlanner* GetPlanner() const;

	/**
	 * @brief Gets the component responsible for selecting this agent's active goal.
	 *
	 * @return Reasoner component owned by this controller.
	 */
	UReasoner* GetReasoner() const;

	/**
	 * @brief Stores a claimed cover slot until this controller leaves cover or unpossesses its agent.
	 *
	 * @param Subsystem Smart Object subsystem that owns the claim.
	 * @param ClaimHandle Valid cover slot claim transferred from FindCover.
	 * @param InCoverTags Activity tags authored on the claimed Smart Object slot.
	 */
	void SetCoverClaim(
		USmartObjectSubsystem* Subsystem,
		const FSmartObjectClaimHandle& ClaimHandle,
		const FGameplayTagContainer& InCoverTags);

	/**
	 * @brief Reports whether this controller currently owns a claimed or occupied cover slot.
	 *
	 * @return True while a valid Smart Object cover reservation is retained.
	 */
	bool HasCoverClaim() const;

	/** Releases the cover slot currently reserved by this controller. */
	void ReleaseCoverClaim();

	/**
	 * @brief Changes the agent's active lean direction for animation and cover behavior.
	 *
	 * @param NewDirection Direction the agent is actively leaning.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Cover")
	void SetCoverLeanDirection(ECoverLeanDirection NewDirection);

	/**
	 * @brief Gets the first valid enemy actor currently visible to this controller.
	 *
	 * @return Current visible enemy target, or nullptr when no enemy is visible.
	 */
	AActor* GetCurrentEnemyTarget() const;

	/**
	 * @brief Gets every enemy actor currently visible to this controller.
	 *
	 * @return Controller-owned visible-enemy list. Callers must ignore invalid entries.
	 */
	const TArray<TObjectPtr<AActor>>& GetSeenEnemies() const;

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

	/**
	 * @brief Gets the most recent grenade throw trajectory selected by grenade eligibility evaluation.
	 *
	 * @param OutSolution Receives the selected grenade throw solution when available.
	 * @return True when a valid grenade throw solution is currently cached.
	 */
	bool GetCurrentGrenadeThrowSolution(FGrenadeThrowSolution& OutSolution) const;

protected:

	/**
	 * @brief Pulls agent customization from the possessed pawn and applies controller settings.
	 *
	 * @param InPawn Pawn newly possessed by this controller.
	 */
	virtual void OnPossess(APawn* InPawn) override;

	/** Clears runtime perception state before this controller releases its pawn. */
	virtual void OnUnPossess() override;

	/** Applies action and perception configuration from the possessed agent customization asset. */
	void ApplyAgentCustomization();

	/** Mirrors relevant ability-system state tags into the planner. */
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

	/** Updates runtime combat state from this agent's perception knowledge. */
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

	/** Gets grenade settings for the inventory-selected grenade type. */
	const FGrenadeProperties* GetCurrentGrenadeProperties() const;

	/** Updates grenade-related GOAP state from enemy clustering, range, cooldown, and collateral checks. */
	void RefreshGrenadeDecisionState();

	/** Finds the best seen-or-known enemy cluster center for a grenade throw. */
	bool FindBestGrenadeClusterCenter(FVector& OutClusterCenter, int32& OutClusterEnemyCount) const;

	/** Checks whether the configured grenade throw trajectory can reach a target point. */
	bool CanReachGrenadeTarget(const FVector& TargetLocation, const FGrenadeProperties& GrenadeProperties, FGrenadeThrowSolution& OutSolution) const;

	/** Checks whether a grenade target point would include friendly agents inside the collateral radius. */
	bool HasFriendlyInGrenadeCollateralRadius(const FVector& TargetLocation, const FGrenadeProperties& GrenadeProperties) const;

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
	TObjectPtr<UPlanner> Planner;

	/** Chooses the active goal consumed by the planner. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UReasoner> Reasoner;

	/** Activity tags copied from the currently claimed Smart Object cover slot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Cover")
	FGameplayTagContainer CurrentCoverTags;

	/** Direction in which this agent is actively leaning from cover. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Cover")
	ECoverLeanDirection CoverLeanDirection = ECoverLeanDirection::None;

	/** Smart Object subsystem that owns the current cover claim. */
	TWeakObjectPtr<USmartObjectSubsystem> CoverSmartObjectSubsystem;

	/** Cover slot retained while this controller's agent occupies cover. */
	FSmartObjectClaimHandle CoverClaimHandle;

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

	/** Cached grenade throw solution selected by the most recent grenade decision refresh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Grenade")
	FGrenadeThrowSolution CurrentGrenadeThrowSolution;

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
