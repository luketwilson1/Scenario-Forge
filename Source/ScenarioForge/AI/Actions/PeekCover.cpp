// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file PeekCover.cpp
 * @brief Implements cover peeking and perception-backed enemy reacquisition.
 */

#include "PeekCover.h"

#include "Agent.h"
#include "AgentAIController.h"
#include "Components/ArrowComponent.h"
#include "CoverActor.h"
#include "Navigation/PathFollowingComponent.h"
#include "Planner.h"
#include "ScenarioForgeGameplayTags.h"
#include "TimerManager.h"

UPeekCover::UPeekCover()
{
	bCanBeInterrupted = true;
	TruePreconditions.AddTag(TAG_State_InCover.GetTag());
	TruePreconditions.AddTag(TAG_State_RemembersEnemy.GetTag());
	FalsePreconditions.AddTag(TAG_State_Dead.GetTag());

	/** Planning prediction only; runtime success still requires perception and range to confirm both states. */
	AddedEffects.AddTag(TAG_State_SeesEnemy.GetTag());
	AddedEffects.AddTag(TAG_State_InWeaponRange.GetTag());
	RemovedEffects.AddTag(TAG_State_RemembersEnemy.GetTag());
}

EActionResult UPeekCover::Execute(UPlanner* Planner)
{
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	AAgent* Agent = Controller ? Cast<AAgent>(Controller->GetPawn()) : nullptr;
	ACoverActor* CoverActor = Controller ? Cast<ACoverActor>(Controller->GetClaimedCoverActor()) : nullptr;
	UPathFollowingComponent* PathFollowingComponent = Controller ? Controller->GetPathFollowingComponent() : nullptr;
	TArray<UArrowComponent*> PeekPoints;
	if (CoverActor)
	{
		CoverActor->GetEnabledPeekPoints(PeekPoints);
	}
	if (!Planner || !Controller || !Agent || !CoverActor || !PathFollowingComponent || PeekPoints.IsEmpty())
	{
		return EActionResult::Failed;
	}

	UArrowComponent* PeekPoint = PeekPoints[FMath::RandHelper(PeekPoints.Num())];
	ExecutingPlanner = Planner;
	Phase = EPeekPhase::MovingToPeek;
	MoveCompletedDelegateHandle = PathFollowingComponent->OnRequestFinished.AddUObject(this, &UPeekCover::HandleMoveCompleted);

	const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
		PeekPoint->GetComponentLocation(),
		-1.0f,
		true,
		true,
		true,
		true,
		nullptr,
		false);

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		BeginSightCheck();
		return EActionResult::Running;
	}
	if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		MoveRequestID = Controller->GetCurrentMoveRequestID();
		return EActionResult::Running;
	}

	Cleanup();
	return EActionResult::Failed;
}

bool UPeekCover::Interrupt(UPlanner* Planner)
{
	if (!Planner || ExecutingPlanner.Get() != Planner)
	{
		return false;
	}

	AAgentAIController* Controller = Cast<AAgentAIController>(Planner->GetOwner());
	if (AAgent* Agent = Controller ? Cast<AAgent>(Controller->GetPawn()) : nullptr)
	{
		Agent->SetPeekingFromCover(false);
	}

	Cleanup();
	if (Controller)
	{
		Controller->StopMovement();
	}
	return true;
}

void UPeekCover::BeginDestroy()
{
	Cleanup();
	Super::BeginDestroy();
}

void UPeekCover::HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (!MoveRequestID.IsValid() || !RequestID.IsEquivalent(MoveRequestID))
	{
		return;
	}

	MoveRequestID = FAIRequestID();
	if (Phase == EPeekPhase::MovingToPeek)
	{
		if (Result.IsSuccess())
		{
			BeginSightCheck();
		}
		else
		{
			FinishAction(EActionResult::Failed);
		}
	}
	else if (Phase == EPeekPhase::ReturningToCover)
	{
		FinishAction(EActionResult::Failed);
	}
}

void UPeekCover::BeginSightCheck()
{
	UPlanner* Planner = ExecutingPlanner.Get();
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	AAgent* Agent = Controller ? Cast<AAgent>(Controller->GetPawn()) : nullptr;
	UWorld* World = Agent ? Agent->GetWorld() : nullptr;
	if (!World || !Agent)
	{
		FinishAction(EActionResult::Failed);
		return;
	}

	Phase = EPeekPhase::WaitingForSight;
	Agent->SetPeekingFromCover(true);
	SightCheckDeadline = World->GetTimeSeconds() + FMath::Max(0.05f, ReacquireTimeout);
	World->GetTimerManager().SetTimer(
		SightCheckTimerHandle,
		this,
		&UPeekCover::CheckForReacquiredEnemy,
		FMath::Max(0.01f, ReacquirePollInterval),
		true,
		0.0f);
}

void UPeekCover::CheckForReacquiredEnemy()
{
	UPlanner* Planner = ExecutingPlanner.Get();
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	UWorld* World = Controller ? Controller->GetWorld() : nullptr;
	if (!Planner || !Controller || !World)
	{
		FinishAction(EActionResult::Failed);
		return;
	}

	Controller->RefreshAttackRangeStates();
	const bool bSeesEnemy = Planner->CurrentStates.HasTagExact(TAG_State_SeesEnemy.GetTag());
	const bool bInWeaponRange = Planner->CurrentStates.HasTagExact(TAG_State_InWeaponRange.GetTag());
	if (bSeesEnemy && bInWeaponRange)
	{
		FinishAction(EActionResult::Succeeded);
		return;
	}

	if (World->GetTimeSeconds() >= SightCheckDeadline)
	{
		ReturnToCoverAfterFailure();
	}
}

void UPeekCover::ReturnToCoverAfterFailure()
{
	UPlanner* Planner = ExecutingPlanner.Get();
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	AAgent* Agent = Controller ? Cast<AAgent>(Controller->GetPawn()) : nullptr;
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(SightCheckTimerHandle);
	}
	if (Agent)
	{
		Agent->SetPeekingFromCover(false);
	}

	FVector CoverLocation;
	if (!Controller || !Controller->GetClaimedCoverLocation(CoverLocation))
	{
		FinishAction(EActionResult::Failed);
		return;
	}

	Phase = EPeekPhase::ReturningToCover;
	const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
		CoverLocation,
		-1.0f,
		true,
		true,
		true,
		true,
		nullptr,
		false);
	if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		MoveRequestID = Controller->GetCurrentMoveRequestID();
	}
	else
	{
		FinishAction(EActionResult::Failed);
	}
}

void UPeekCover::FinishAction(EActionResult Result)
{
	UPlanner* Planner = ExecutingPlanner.Get();
	Cleanup();
	if (Planner)
	{
		Planner->CompleteActiveAction(Result);
	}
}

void UPeekCover::Cleanup()
{
	UPlanner* Planner = ExecutingPlanner.Get();
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	if (UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(SightCheckTimerHandle);
	}
	if (UPathFollowingComponent* PathFollowingComponent = Controller ? Controller->GetPathFollowingComponent() : nullptr)
	{
		PathFollowingComponent->OnRequestFinished.Remove(MoveCompletedDelegateHandle);
	}

	ExecutingPlanner.Reset();
	MoveRequestID = FAIRequestID();
	MoveCompletedDelegateHandle.Reset();
	SightCheckTimerHandle.Invalidate();
	SightCheckDeadline = 0.0;
	Phase = EPeekPhase::None;
}
