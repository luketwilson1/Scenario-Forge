// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file TacticalPositioningComponent.cpp
 * @brief Implements tactical movement intent storage for AI agents.
 */

#include "TacticalPositioningComponent.h"

#include "Agent.h"
#include "AgentAIController.h"
#include "AgentCustomization.h"
#include "Area.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Evaluators/FiringPositionEval.h"
#include "FiringPosition.h"
#include "Navigation/PathFollowingComponent.h"
#include "Public/Squad.h"
#include "Zone.h"

/**
 * @brief Initializes ticking so future position evaluators can run independently of GOAP actions.
 */
UTacticalPositioningComponent::UTacticalPositioningComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

/**
 * @brief Sets the active tactical movement evaluation mode.
 *
 * @param NewMovementMode Tactical movement mode to activate.
 */
void UTacticalPositioningComponent::SetMovementMode(ETacticalMovementMode NewMovementMode)
{
	if (MovementMode == NewMovementMode)
	{
		return;
	}

	MovementMode = NewMovementMode;
	bWaitingForMoveCompletion = false;
	ClearDesiredDestination();
	bCoverHoldTimerActive = false;

	if ((MovementMode == ETacticalMovementMode::Cover || MovementMode == ETacticalMovementMode::Uncover) && GEngine)
	{
		const AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetOwner());
		const AAgent* Agent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
		const FString AgentName = IsValid(Agent) ? Agent->GetName() : TEXT("Unknown Agent");
		const TCHAR* ModeName = MovementMode == ETacticalMovementMode::Cover ? TEXT("Cover") : TEXT("Uncover");
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Green, FString::Printf(TEXT("%s entered %s mode"), *AgentName, ModeName));
	}

	ScheduleImmediateReposition();
}

/**
 * @brief Gets the active tactical movement evaluation mode.
 *
 * @return Current movement mode.
 */
ETacticalMovementMode UTacticalPositioningComponent::GetMovementMode() const
{
	return MovementMode;
}

/**
 * @brief Sets the actor that tactical position scoring should consider.
 *
 * @param NewTargetActor Actor used as the positioning target.
 */
void UTacticalPositioningComponent::SetTargetActor(AActor* NewTargetActor)
{
	if (TargetActor.Get() == NewTargetActor)
	{
		return;
	}

	TargetActor = NewTargetActor;
	if (MovementMode == ETacticalMovementMode::Cover && (bWaitingForMoveCompletion || bHasDesiredDestination || bCoverHoldTimerActive))
	{
		return;
	}

	bWaitingForMoveCompletion = false;
	ScheduleImmediateReposition();
}

/**
 * @brief Gets the actor tactical position scoring should consider.
 *
 * @return Current target actor.
 */
AActor* UTacticalPositioningComponent::GetTargetActor() const
{
	return TargetActor;
}

/**
 * @brief Clears the current tactical target actor.
 */
void UTacticalPositioningComponent::ClearTargetActor()
{
	if (!TargetActor)
	{
		return;
	}

	TargetActor = nullptr;
	bWaitingForMoveCompletion = false;
	ClearDesiredDestination();
	bCoverHoldTimerActive = false;
	ScheduleImmediateReposition();
}

/**
 * @brief Stores a selected tactical destination.
 *
 * @param NewDestination World-space destination selected by tactical evaluation.
 */
void UTacticalPositioningComponent::SetDesiredDestination(const FVector& NewDestination)
{
	DesiredDestination = NewDestination;
	bHasDesiredDestination = true;
}

/**
 * @brief Gets the selected tactical destination when available.
 *
 * @param OutDestination Destination value when one is active.
 * @return True when a desired destination is available.
 */
bool UTacticalPositioningComponent::GetDesiredDestination(FVector& OutDestination) const
{
	if (!bHasDesiredDestination)
	{
		return false;
	}

	OutDestination = DesiredDestination;
	return true;
}

/**
 * @brief Clears the selected tactical destination.
 */
void UTacticalPositioningComponent::ClearDesiredDestination()
{
	DesiredDestination = FVector::ZeroVector;
	bHasDesiredDestination = false;
}

/**
 * @brief Future hook for continuous tactical position evaluation.
 *
 * @param DeltaTime Seconds elapsed since the previous tick.
 * @param TickType Tick type supplied by Unreal.
 * @param ThisTickFunction Tick function metadata supplied by Unreal.
 */
void UTacticalPositioningComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UWorld* World = GetWorld();
	if (!World || MovementMode == ETacticalMovementMode::None)
	{
		return;
	}

	if (bWaitingForMoveCompletion)
	{
		const AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetOwner());
		const UPathFollowingComponent* PathFollowingComponent = AgentAIController ? AgentAIController->GetPathFollowingComponent() : nullptr;

		if (HasReachedDesiredDestination())
		{
			bWaitingForMoveCompletion = false;
			ScheduleNextReposition();
		}
		else if (PathFollowingComponent
			&& PathFollowingComponent->GetStatus() != EPathFollowingStatus::Moving
			&& PathFollowingComponent->GetStatus() != EPathFollowingStatus::Waiting)
		{
			bWaitingForMoveCompletion = false;
			if (MovementMode == ETacticalMovementMode::Cover)
			{
				ScheduleNextReposition();
			}
			else
			{
				ClearDesiredDestination();
				ScheduleImmediateReposition();
			}
		}

		return;
	}

	if (World->GetTimeSeconds() >= NextRepositionTime)
	{
		if (MovementMode == ETacticalMovementMode::Cover && bCoverHoldTimerActive)
		{
			SetMovementMode(ETacticalMovementMode::Uncover);
		}

		if ((MovementMode == ETacticalMovementMode::Combat
				|| MovementMode == ETacticalMovementMode::Cover
				|| MovementMode == ETacticalMovementMode::Uncover)
			&& !EvaluateAndMoveToBestFiringPosition())
		{
			ScheduleImmediateReposition();
		}
	}
}

bool UTacticalPositioningComponent::EvaluateAndMoveToBestFiringPosition()
{
	AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetOwner());
	AAgent* Agent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	ASquad* Squad = FindOwningSquad();
	if (!AgentAIController || !Agent || !Squad || !Squad->Zone)
	{
		return false;
	}

	AFiringPosition* BestPosition = nullptr;
	float BestScore = 0.0f;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	const FVector AgentLocation = Agent->GetActorLocation();

	for (AArea* Area : Squad->Zone->Areas)
	{
		if (!IsValid(Area))
		{
			continue;
		}

		for (AFiringPosition* FiringPosition : Area->FiringPositions)
		{
			if (!IsValid(FiringPosition))
			{
				continue;
			}

			const float CandidateDistanceSquared = FVector::DistSquared2D(AgentLocation, FiringPosition->GetActorLocation());
			const float CandidateScore = ScoreFiringPosition(FiringPosition);
			if (CandidateScore <= 0.0f)
			{
				continue;
			}

			if (CandidateScore > BestScore
				|| (FMath::IsNearlyEqual(CandidateScore, BestScore) && CandidateDistanceSquared < BestDistanceSquared))
			{
				BestScore = CandidateScore;
				BestDistanceSquared = CandidateDistanceSquared;
				BestPosition = FiringPosition;
			}
		}
	}

	if (BestPosition)
	{
		SetDesiredDestination(BestPosition->GetActorLocation());
		const float AcceptanceRadius = GetRepositionAcceptanceRadius();
		const EPathFollowingRequestResult::Type MoveResult = AgentAIController->MoveToLocation(DesiredDestination, AcceptanceRadius, true, true, true, true);

		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			ClearDesiredDestination();
			bWaitingForMoveCompletion = false;
			return false;
		}

		bWaitingForMoveCompletion = MoveResult == EPathFollowingRequestResult::RequestSuccessful;
		if (!bWaitingForMoveCompletion)
		{
			ScheduleNextReposition();
		}
		return true;
	}

	return false;
}

bool UTacticalPositioningComponent::HasReachedDesiredDestination() const
{
	const AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetOwner());
	const AAgent* Agent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	if (!Agent || !bHasDesiredDestination)
	{
		return false;
	}

	return FVector::DistSquared2D(Agent->GetActorLocation(), DesiredDestination) <= FMath::Square(GetRepositionAcceptanceRadius());
}

float UTacticalPositioningComponent::GetRepositionAcceptanceRadius() const
{
	const AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetOwner());
	const AAgent* Agent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	const UAgentCustomization* AgentCustomization = Agent ? Agent->GetAgentCustomization() : nullptr;
	if (!AgentCustomization)
	{
		return 150.0f;
	}

	return FMath::Max(0.0f, AgentCustomization->GetResolvedEngageProperties().RepositionAcceptanceRadius);
}

ASquad* UTacticalPositioningComponent::FindOwningSquad() const
{
	const AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetOwner());
	const AAgent* Agent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	const UWorld* World = GetWorld();
	if (!Agent || !World)
	{
		return nullptr;
	}

	for (TActorIterator<ASquad> It(World); It; ++It)
	{
		ASquad* Squad = *It;
		if (IsValid(Squad) && Squad->Agents.Contains(Agent))
		{
			return Squad;
		}
	}

	return nullptr;
}

float UTacticalPositioningComponent::ScoreFiringPosition(const AFiringPosition* CandidatePosition) const
{
	const AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetOwner());
	const AAgent* Agent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	const UAgentCustomization* AgentCustomization = Agent ? Agent->GetAgentCustomization() : nullptr;
	if (!AgentCustomization || !CandidatePosition)
	{
		return 0.0f;
	}

	const FTacticalMovementModeEvaluatorSet* EvaluatorSet = AgentCustomization->GetResolvedTacticalPositionEvaluators().Find(MovementMode);
	if (!EvaluatorSet)
	{
		return 0.0f;
	}

	float TotalScore = 0.0f;
	for (const FTacticalPositionEvaluatorConfig& EvaluatorConfig : EvaluatorSet->Evaluators)
	{
		const UFiringPositionEval* Evaluator = EvaluatorConfig.EvaluatorClass
			? EvaluatorConfig.EvaluatorClass->GetDefaultObject<UFiringPositionEval>()
			: nullptr;
		if (!Evaluator)
		{
			continue;
		}

		const float RawScore = Evaluator->EvaluateRawScoreWithDebug(this, CandidatePosition, EvaluatorConfig.bDebugDraw);
		TotalScore += EvaluatorConfig.Points * RawScore;
	}

	return TotalScore;
}

void UTacticalPositioningComponent::ScheduleNextReposition()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		NextRepositionTime = 0.0f;
		return;
	}

	const AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetOwner());
	const AAgent* Agent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	const UAgentCustomization* AgentCustomization = Agent ? Agent->GetAgentCustomization() : nullptr;
	if (!AgentCustomization)
	{
		bCoverHoldTimerActive = false;
		NextRepositionTime = World->GetTimeSeconds();
		return;
	}

	float MinimumDelay = 0.0f;
	float MaximumDelay = 0.0f;
	bCoverHoldTimerActive = false;
	if (MovementMode == ETacticalMovementMode::Combat)
	{
		const FEngageProperties& EngageProperties = AgentCustomization->GetResolvedEngageProperties();
		MinimumDelay = FMath::Max(0.0f, EngageProperties.MinimumRepositionDelay);
		MaximumDelay = FMath::Max(MinimumDelay, EngageProperties.MaximumRepositionDelay);
	}
	else if (MovementMode == ETacticalMovementMode::Cover)
	{
		const FCoverProperties& CoverProperties = AgentCustomization->GetResolvedCoverProperties();
		MinimumDelay = FMath::Max(0.0f, CoverProperties.MinimumHideBehindCoverTime);
		MaximumDelay = FMath::Max(MinimumDelay, CoverProperties.MaximumHideBehindCoverTime);
		bCoverHoldTimerActive = true;
	}

	NextRepositionTime = (MovementMode == ETacticalMovementMode::Combat || MovementMode == ETacticalMovementMode::Cover)
		? World->GetTimeSeconds() + FMath::FRandRange(MinimumDelay, MaximumDelay)
		: TNumericLimits<float>::Max();
}

void UTacticalPositioningComponent::ScheduleImmediateReposition()
{
	bCoverHoldTimerActive = false;
	const UWorld* World = GetWorld();
	NextRepositionTime = World ? World->GetTimeSeconds() : 0.0f;
}
