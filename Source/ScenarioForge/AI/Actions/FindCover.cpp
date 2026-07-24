// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FindCover.cpp
 * @brief Implements Smart Object cover discovery, claiming, and movement.
 */

#include "FindCover.h"

#include "AIController.h"
#include "Agent.h"
#include "AgentAIController.h"
#include "Engine/Engine.h"
#include "Navigation/PathFollowingComponent.h"
#include "Planner.h"
#include "ScenarioForgeGameplayTags.h"
#include "SmartObjectComponent.h"
#include "SmartObjectRequestTypes.h"
#include "SmartObjectSubsystem.h"

namespace
{
	struct FCoverCandidate
	{
		FSmartObjectRequestResult RequestResult;
		FVector SlotLocation = FVector::ZeroVector;
		double DistanceSquared = TNumericLimits<double>::Max();
	};

	/** Returns true when world geometry blocks every seen enemy's view of the agent at the candidate slot. */
	bool IsHiddenFromSeenEnemies(
		const AAgentAIController* Controller,
		const APawn* Pawn,
		UWorld* World,
		const FVector& CandidateLocation)
	{
		if (!Controller || !Pawn || !World)
		{
			return false;
		}

		const FVector PawnViewOffset = Pawn->GetPawnViewLocation() - Pawn->GetActorLocation();
		const FVector CandidateViewLocation = CandidateLocation + PawnViewOffset;
		for (AActor* SeenEnemy : Controller->GetSeenEnemies())
		{
			if (!IsValid(SeenEnemy))
			{
				continue;
			}

			FVector EnemyViewLocation;
			FRotator EnemyViewRotation;
			SeenEnemy->GetActorEyesViewPoint(EnemyViewLocation, EnemyViewRotation);

			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FindCoverVisibility), true);
			QueryParams.AddIgnoredActor(Pawn);
			for (AActor* OtherSeenEnemy : Controller->GetSeenEnemies())
			{
				if (IsValid(OtherSeenEnemy))
				{
					QueryParams.AddIgnoredActor(OtherSeenEnemy);
				}
			}

			FHitResult BlockingHit;
			if (!World->LineTraceSingleByChannel(
				BlockingHit,
				EnemyViewLocation,
				CandidateViewLocation,
				ECC_Visibility,
				QueryParams))
			{
				return false;
			}
		}

		return true;
	}
}

/**
 * @brief Configures cover planning tags and the BP_Cover class reference.
 */
UFindCover::UFindCover()
{
	bCanBeInterrupted = true;
	ConcurrentFirePolicy = EConcurrentFirePolicy::VisibleTargets;
	TruePreconditions.AddTag(TAG_State_FiredUpon.GetTag());
	FalsePreconditions.AddTag(TAG_State_InCover.GetTag());
	AddedEffects.AddTag(TAG_State_InCover.GetTag());
	CoverSmartObjectClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/BP_Cover.BP_Cover_C")));
}

/**
 * @brief Finds, claims, and moves toward the nearest claimable BP_Cover slot.
 *
 * @param Planner Planner executing this action.
 * @return Running when movement starts, Succeeded when already at cover, Failed when cover cannot be reserved or reached, or Invalid for a bad execution context.
 */
EActionResult UFindCover::Execute(UPlanner* Planner)
{
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	USmartObjectSubsystem* Subsystem = World ? World->GetSubsystem<USmartObjectSubsystem>() : nullptr;
	UClass* CoverClass = CoverSmartObjectClass.LoadSynchronous();
	if (!Planner || !Controller || !Pawn || !Subsystem || !CoverClass || SearchRadius <= 0.0f)
	{
		return EActionResult::Invalid;
	}

	/** Do not replace and release the reservation from an earlier FindCover execution that is still running. */
	if (Controller->HasCoverClaim())
	{
		return EActionResult::Running;
	}

	FSmartObjectRequestFilter Filter;
	/** Include every matching slot so this action can explicitly reject claimed or occupied entries. */
	Filter.bShouldIncludeClaimedSlots = true;
	const FBox SearchBounds = FBox::BuildAABB(Pawn->GetActorLocation(), FVector(SearchRadius));
	const FSmartObjectRequest Request(SearchBounds, Filter);

	TArray<FSmartObjectRequestResult> Results;
	if (!Subsystem->FindSmartObjects_BP(Request, Results, Pawn))
	{
		return EActionResult::Failed;
	}

	Results.RemoveAll([Subsystem, CoverClass](const FSmartObjectRequestResult& Result)
	{
		const USmartObjectComponent* Component = Subsystem->GetSmartObjectComponentByRequestResult(Result);
		return !Component || !Component->GetOwner() || !Component->GetOwner()->IsA(CoverClass);
	});

	const FVector PawnLocation = Pawn->GetActorLocation();
	TArray<FCoverCandidate> Candidates;
	Candidates.Reserve(Results.Num());
	for (const FSmartObjectRequestResult& Result : Results)
	{
		const TOptional<FTransform> SlotTransform = Subsystem->GetSlotTransform(Result);
		if (!SlotTransform.IsSet()
			|| !IsHiddenFromSeenEnemies(Controller, Pawn, World, SlotTransform->GetLocation()))
		{
			continue;
		}

		FCoverCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.RequestResult = Result;
		Candidate.SlotLocation = SlotTransform->GetLocation();
		Candidate.DistanceSquared = FVector::DistSquared(PawnLocation, Candidate.SlotLocation);
	}

	Candidates.Sort([](const FCoverCandidate& Left, const FCoverCandidate& Right)
	{
		return Left.DistanceSquared < Right.DistanceSquared;
	});

	FVector CoverSlotLocation = FVector::ZeroVector;
	for (const FCoverCandidate& Candidate : Candidates)
	{
		const FSmartObjectSlotHandle SlotHandle = Candidate.RequestResult.SlotHandle;

		/** A cover destination is available only when its runtime slot is explicitly free. */
		if (Subsystem->GetSlotState(SlotHandle) != ESmartObjectSlotState::Free)
		{
			continue;
		}

		/** Claiming is the authoritative check in case another agent reserved the slot after the free-state check. */
		CoverClaimHandle = Subsystem->MarkSlotAsClaimed(SlotHandle, ESmartObjectClaimPriority::Normal);
		if (!CoverClaimHandle.IsValid())
		{
			continue;
		}

		if (Subsystem->GetSlotState(SlotHandle) == ESmartObjectSlotState::Claimed)
		{
			CoverSlotLocation = Candidate.SlotLocation;
			break;
		}

		Subsystem->MarkSlotAsFree(CoverClaimHandle);
		CoverClaimHandle = FSmartObjectClaimHandle();
	}

	if (!CoverClaimHandle.IsValid())
	{
		if (!Candidates.IsEmpty() && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Yellow,
				FString::Printf(TEXT("%s: Cover spot occupied"), *GetNameSafe(Pawn)));
		}
		return EActionResult::Failed;
	}

	UE_LOG(LogTemp, Display, TEXT("FindCover[%s]: reserved slot %s."), *GetNameSafe(Pawn), *LexToString(CoverClaimHandle));

	ExecutingPlanner = Planner;
	SmartObjectSubsystem = Subsystem;

	UPathFollowingComponent* PathFollowingComponent = Controller->GetPathFollowingComponent();
	if (!PathFollowingComponent)
	{
		CleanupMove();
		return EActionResult::Invalid;
	}

	/** Transfer the reservation before movement so replanning cannot destroy this action and free the destination. */
	TransferClaimToController();
	MoveCompletedDelegateHandle = PathFollowingComponent->OnRequestFinished.AddUObject(this, &UFindCover::HandleMoveCompleted);
	if (AAgent* Agent = Cast<AAgent>(Pawn))
	{
		Agent->DisableAutomaticMovementFacing();
	}
	/** A negative acceptance radius delegates arrival tolerance to Unreal's path-following defaults. */
	const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
		CoverSlotLocation,
		-1.0f,
		true,
		true,
		true,
		true,
		nullptr,
		false);

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		/** The successful state update must not interrupt this action and release its retained cover claim. */
		bCanBeInterrupted = false;
		Planner->AddCurrentState(TAG_State_InCover.GetTag());
		CleanupMove();
		return EActionResult::Succeeded;
	}
	else if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		MoveRequestID = Controller->GetCurrentMoveRequestID();
		return EActionResult::Running;
	}

	ReleaseControllerClaim();
	CleanupMove();
	return EActionResult::Failed;
}

bool UFindCover::Interrupt(UPlanner* Planner)
{
	if (!ExecutingPlanner.IsValid() || ExecutingPlanner.Get() != Planner)
	{
		return false;
	}

	AAgentAIController* Controller = Cast<AAgentAIController>(Planner->GetOwner());
	if (!Controller)
	{
		return false;
	}

	/** Unbind first so aborting movement cannot report completion into the planner being preempted. */
	if (UPathFollowingComponent* PathFollowingComponent = Controller->GetPathFollowingComponent())
	{
		PathFollowingComponent->OnRequestFinished.Remove(MoveCompletedDelegateHandle);
	}
	MoveCompletedDelegateHandle.Reset();
	Controller->StopMovement();
	ReleaseControllerClaim();
	CleanupMove();
	return true;
}

/**
 * @brief Finalizes cover movement and transfers successful claims to the controller.
 *
 * @param RequestID Identifier of the completed move request.
 * @param Result Path-following result reported by the controller.
 */
void UFindCover::HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	/** Ignore aborted requests emitted synchronously while MoveToLocation is starting the cover move. */
	if (!MoveRequestID.IsValid() || !RequestID.IsEquivalent(MoveRequestID))
	{
		return;
	}

	UPlanner* Planner = ExecutingPlanner.Get();
	const bool bReachedCover = Result.IsSuccess();
	if (!bReachedCover)
	{
		ReleaseControllerClaim();
	}
	else if (Planner)
	{
		/** Commit successful cover context before State.InCover causes goal selection to run again. */
		bCanBeInterrupted = false;
		Planner->AddCurrentState(TAG_State_InCover.GetTag());
	}

	CleanupMove();
	if (Planner)
	{
		Planner->CompleteActiveAction(bReachedCover ? EActionResult::Succeeded : EActionResult::Failed);
	}
}

/**
 * @brief Releases temporary movement state before Unreal destroys this action object.
 */
void UFindCover::BeginDestroy()
{
	CleanupMove();
	Super::BeginDestroy();
}

/**
 * @brief Unbinds path completion and releases any claim still owned by this action.
 */
void UFindCover::CleanupMove()
{
	if (UPlanner* Planner = ExecutingPlanner.Get())
	{
		if (AAIController* Controller = Cast<AAIController>(Planner->GetOwner()))
		{
			if (UPathFollowingComponent* PathFollowingComponent = Controller->GetPathFollowingComponent())
			{
				PathFollowingComponent->OnRequestFinished.Remove(MoveCompletedDelegateHandle);
			}
		}
	}

	if (USmartObjectSubsystem* Subsystem = SmartObjectSubsystem.Get(); Subsystem && CoverClaimHandle.IsValid())
	{
		Subsystem->MarkSlotAsFree(CoverClaimHandle);
	}

	ExecutingPlanner.Reset();
	SmartObjectSubsystem.Reset();
	CoverClaimHandle = FSmartObjectClaimHandle();
	MoveRequestID = FAIRequestID();
	MoveCompletedDelegateHandle.Reset();
}

/**
 * @brief Transfers the reserved cover claim from this action to its AI controller.
 */
void UFindCover::TransferClaimToController()
{
	UPlanner* Planner = ExecutingPlanner.Get();
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	USmartObjectSubsystem* Subsystem = SmartObjectSubsystem.Get();
	if (Controller && Subsystem && CoverClaimHandle.IsValid())
	{
		Controller->SetCoverClaim(Subsystem, CoverClaimHandle);
		CoverClaimHandle = FSmartObjectClaimHandle();
	}
}

/**
 * @brief Releases the cover reservation retained by the controller after movement fails.
 */
void UFindCover::ReleaseControllerClaim()
{
	UPlanner* Planner = ExecutingPlanner.Get();
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	if (Controller)
	{
		Controller->ReleaseCoverClaim();
	}
}
