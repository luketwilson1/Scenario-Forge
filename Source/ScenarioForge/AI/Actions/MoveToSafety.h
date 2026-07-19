// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file MoveToSafety.h
 * @brief Declares the GOAP action that uses EQS to escape active explosive radii.
 */

#pragma once

#include "CoreMinimal.h"
#include "Action.h"
#include "AITypes.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "MoveToSafety.generated.h"

struct FPathFollowingResult;
class UEnvQuery;

/** @brief Queries a reachable safe point and moves the agent outside every active grenade danger radius. */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UMoveToSafety : public UAction
{
	GENERATED_BODY()

public:
	/** Initializes grenade-danger preconditions and self-preservation effects. */
	UMoveToSafety();

	/** Starts the configured safety EQS query. */
	virtual EActionResult Execute(UPlanner* Planner) override;

	/** Aborts the active EQS or movement request before another action starts. */
	virtual bool Interrupt(UPlanner* Planner) override;

	/** Clears query and movement callbacks before this runtime action is destroyed. */
	virtual void BeginDestroy() override;

private:
	/** Handles completion of the safety EQS query. */
	void HandleQueryFinished(TSharedPtr<FEnvQueryResult> Result);

	/** Handles completion of the navigation request to the selected safety point. */
	void HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

	/** Stops query/movement work and removes bound delegates. */
	void Cleanup(bool bStopMovement);

	/** Builds a transient grid/path/distance EQS template for this escape request. */
	UEnvQuery* BuildSafetyQuery(float SafeDistance, float GridHalfSize);

	/** Planner that owns this asynchronous action. */
	TWeakObjectPtr<UPlanner> ExecutingPlanner;

	/** Running EQS request identifier, or INDEX_NONE when no query is active. */
	int32 QueryID = INDEX_NONE;

	/** Navigation request associated with the selected safety point. */
	FAIRequestID MoveRequestID;

	/** Delegate binding used to receive path-following completion. */
	FDelegateHandle MoveCompletedDelegateHandle;

	/** Runtime-owned EQS template retained until the query completes. */
	UPROPERTY(Transient)
	TObjectPtr<UEnvQuery> RuntimeSafetyQuery;
};
