// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Action.h
 * @brief Declares the base runtime object for GOAP action execution.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Action.generated.h"

class UPlanner;

/**
 * @brief Immediate execution result reported by every planner action.
 */
UENUM(BlueprintType)
enum class EActionResult : uint8
{
	/** The action started successfully and is still executing asynchronously. */
	Running,

	/** The action completed successfully. */
	Succeeded,

	/** The action could not complete its intended behavior. */
	Failed,

	/** The action was stopped by an external event before completion. */
	Interrupted,

	/** The action or its execution context was not configured correctly. */
	Invalid
};

/** Controls whether the independent weapon channel may fire while an action is running. */
UENUM(BlueprintType)
enum class EConcurrentFirePolicy : uint8
{
	/** This action requires exclusive control of the agent's weapon/body animation. */
	Disallowed,

	/** Opportunistic bursts are allowed while a valid enemy remains visible and in range. */
	VisibleTargets
};

/**
 * @brief Base object for a planner-selected GOAP action.
 */
UCLASS(Abstract, Blueprintable)
class SCENARIOFORGE_API UAction : public UObject
{
	GENERATED_BODY()

public:
	/** Runtime planning cost assigned from the owning agent's Agent Sheet. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action|Planning")
	float ActionCost = 0.0f;

	/** Whether a goal change may ask this action to stop before it completes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Planning")
	bool bCanBeInterrupted = false;

	/** Whether the controller's weapon channel may run alongside this action. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Combat")
	EConcurrentFirePolicy ConcurrentFirePolicy = EConcurrentFirePolicy::Disallowed;

	/** State tags that must be present before this action can be used. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTagContainer TruePreconditions;

	/** State tags that must be absent before this action can be used. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTagContainer FalsePreconditions;

	/** State tags made true after this action is applied. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTagContainer AddedEffects;

	/** State tags made false after this action is applied. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTagContainer RemovedEffects;

	/**
	 * @brief Executes this action for the supplied planner.
	 *
	 * @param Planner Planner that selected and owns this runtime action.
	 * @return Immediate execution result for this action.
	 */
	UFUNCTION(BlueprintCallable, Category = "Action")
	virtual EActionResult Execute(UPlanner* Planner);

	/**
	 * @brief Attempts to stop this action so the planner may execute a better plan.
	 *
	 * @param Planner Planner that owns the active runtime action.
	 * @return True when all asynchronous behavior was stopped safely. Interruptible actions must override this.
	 */
	virtual bool Interrupt(UPlanner* Planner);
};
