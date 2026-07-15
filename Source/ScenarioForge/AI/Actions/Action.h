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

/**
 * @brief Base object for a planner-selected GOAP action.
 */
UCLASS(Abstract, Blueprintable)
class SCENARIOFORGE_API UAction : public UObject
{
	GENERATED_BODY()

public:
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
};
