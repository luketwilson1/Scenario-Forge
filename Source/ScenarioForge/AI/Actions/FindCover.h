// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FindCover.h
 * @brief Declares the action that moves an agent to an available cover Smart Object slot.
 */

#pragma once

#include "CoreMinimal.h"
#include "Action.h"
#include "AITypes.h"
#include "SmartObjectRuntime.h"
#include "FindCover.generated.h"

struct FPathFollowingResult;
class USmartObjectSubsystem;

/**
 * @brief Claims an available BP_Cover Smart Object slot and moves the agent to it.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UFindCover : public UAction
{
	GENERATED_BODY()

public:
	/** @brief Initializes cover planning tags and the default BP_Cover class reference. */
	UFindCover();

	/**
	 * @brief Finds the nearest available cover slot and starts a navigation request to it.
	 *
	 * @param Planner Planner executing the cover action.
	 * @return Immediate execution result for the cover request.
	 */
	virtual EActionResult Execute(UPlanner* Planner) override;

	/** Stops cover movement and releases its reservation before the planner switches goals. */
	virtual bool Interrupt(UPlanner* Planner) override;

	/** @brief Releases any in-flight claim before this runtime action is destroyed. */
	virtual void BeginDestroy() override;

	/** Maximum distance around the agent searched for cover Smart Objects. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Cover", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float SearchRadius = 5000.0f;

	/** Blueprint class whose Smart Object slots are treated as cover. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Cover")
	TSoftClassPtr<AActor> CoverSmartObjectClass;

private:
	/**
	 * @brief Handles completion of the navigation request started by this action.
	 *
	 * @param RequestID Identifier of the completed navigation request.
	 * @param Result Path-following result reported by the AI controller.
	 */
	void HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

	/** Releases the claimed cover slot and unbinds movement callbacks. */
	void CleanupMove();

	/** Transfers the active slot claim to the controller before navigation begins. */
	void TransferClaimToController();

	/** Releases the cover claim retained by the executing AI controller. */
	void ReleaseControllerClaim();

	/** Planner that started the asynchronous cover movement. */
	TWeakObjectPtr<UPlanner> ExecutingPlanner;

	/** Smart Object subsystem that owns the temporary movement claim. */
	TWeakObjectPtr<USmartObjectSubsystem> SmartObjectSubsystem;

	/** Slot claimed while the agent is navigating toward cover. */
	FSmartObjectClaimHandle CoverClaimHandle;

	/** Navigation request associated with the cover movement. */
	FAIRequestID MoveRequestID;

	/** Delegate binding used to receive path-following completion. */
	FDelegateHandle MoveCompletedDelegateHandle;
};
