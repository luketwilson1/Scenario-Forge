// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FindCover.cpp
 * @brief Implements Smart Object cover discovery, claiming, and movement.
 */

#include "FindCover.h"

#include "AIController.h"
#include "AgentAIController.h"
#include "Engine/Engine.h"
#include "Navigation/PathFollowingComponent.h"
#include "Planner.h"
#include "ScenarioForgeGameplayTags.h"
#include "SmartObjectComponent.h"
#include "SmartObjectRequestTypes.h"
#include "SmartObjectSubsystem.h"

/**
 * @brief Configures cover planning tags and the BP_Cover class reference.
 */
UFindCover::UFindCover()
{
	TruePreconditions.AddTag(TAG_State_SeesEnemy.GetTag());
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
	Results.Sort([Subsystem, PawnLocation](const FSmartObjectRequestResult& Left, const FSmartObjectRequestResult& Right)
	{
		const TOptional<FTransform> LeftTransform = Subsystem->GetSlotTransform(Left);
		const TOptional<FTransform> RightTransform = Subsystem->GetSlotTransform(Right);
		const double LeftDistance = LeftTransform.IsSet() ? FVector::DistSquared(PawnLocation, LeftTransform->GetLocation()) : TNumericLimits<double>::Max();
		const double RightDistance = RightTransform.IsSet() ? FVector::DistSquared(PawnLocation, RightTransform->GetLocation()) : TNumericLimits<double>::Max();
		return LeftDistance < RightDistance;
	});

	FTransform CoverTransform;
	for (const FSmartObjectRequestResult& Result : Results)
	{
		/** A cover destination is available only when its runtime slot is explicitly free. */
		if (Subsystem->GetSlotState(Result.SlotHandle) != ESmartObjectSlotState::Free)
		{
			continue;
		}

		/** Claiming is the authoritative final check in case another request reserved the slot first. */
		CoverClaimHandle = Subsystem->MarkSlotAsClaimed(Result.SlotHandle, ESmartObjectClaimPriority::Normal);
		if (!CoverClaimHandle.IsValid())
		{
			continue;
		}

		if (Subsystem->GetSlotState(Result.SlotHandle) == ESmartObjectSlotState::Claimed
			&& Subsystem->GetSlotTransform(CoverClaimHandle, CoverTransform))
		{
			break;
		}

		Subsystem->MarkSlotAsFree(CoverClaimHandle);
		CoverClaimHandle = FSmartObjectClaimHandle();
	}

	if (!CoverClaimHandle.IsValid())
	{
		if (!Results.IsEmpty() && GEngine)
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
	/** A negative acceptance radius delegates arrival tolerance to Unreal's path-following defaults. */
	const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
		CoverTransform.GetLocation(),
		-1.0f,
		true,
		true,
		true,
		true,
		nullptr,
		false);

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		CleanupMove();
		Planner->AddCurrentState(TAG_State_InCover.GetTag());
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
	CleanupMove();

	if (bReachedCover && Planner)
	{
		Planner->AddCurrentState(TAG_State_InCover.GetTag());
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
