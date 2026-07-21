// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file PeekCover.h
 * @brief Declares the action that exposes an agent from cover to reacquire a remembered enemy.
 */

#pragma once

#include "CoreMinimal.h"
#include "Action.h"
#include "AITypes.h"
#include "PeekCover.generated.h"

struct FPathFollowingResult;

/** Moves to an authored peek point and succeeds only after perception reacquires an enemy in weapon range. */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UPeekCover : public UAction
{
	GENERATED_BODY()

public:
	UPeekCover();

	virtual EActionResult Execute(UPlanner* Planner) override;
	virtual bool Interrupt(UPlanner* Planner) override;
	virtual void BeginDestroy() override;

	/** Maximum time spent waiting at the peek point for sight perception to update. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Cover", meta = (ClampMin = "0.05", Units = "Seconds"))
	float ReacquireTimeout = 0.75f;

	/** Frequency at which perception-backed planner state is checked after reaching the peek point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action|Cover", meta = (ClampMin = "0.01", Units = "Seconds"))
	float ReacquirePollInterval = 0.05f;

private:
	enum class EPeekPhase : uint8
	{
		None,
		MovingToPeek,
		WaitingForSight,
		ReturningToCover
	};

	void HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);
	void BeginSightCheck();
	void CheckForReacquiredEnemy();
	void ReturnToCoverAfterFailure();
	void FinishAction(EActionResult Result);
	void Cleanup();

	TWeakObjectPtr<UPlanner> ExecutingPlanner;
	FAIRequestID MoveRequestID;
	FDelegateHandle MoveCompletedDelegateHandle;
	FTimerHandle SightCheckTimerHandle;
	double SightCheckDeadline = 0.0;
	EPeekPhase Phase = EPeekPhase::None;
};
