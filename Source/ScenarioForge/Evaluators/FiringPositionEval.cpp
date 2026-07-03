// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FiringPositionEval.cpp
 * @brief Implements the default firing position evaluator behavior.
 */

#include "FiringPositionEval.h"

#include "Engine/World.h"
#include "../FiringPosition.h"
#include "GameFramework/Controller.h"
#include "../TacticalPositioningComponent.h"

/**
 * @brief Provides a neutral default raw score for native and Blueprint evaluator subclasses.
 *
 * @param PositioningComponent Tactical positioning component requesting the score.
 * @param CandidatePosition Candidate firing position to score.
 * @return Neutral raw score.
 */
float UFiringPositionEval::EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const
{
	return 0.0f;
}

float URandomLineOfSightFiringPositionEval::EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const
{
	if (!PositioningComponent || !CandidatePosition)
	{
		return 0.0f;
	}

	AActor* TargetActor = PositioningComponent->GetTargetActor();
	if (!TargetActor)
	{
		return 0.0f;
	}

	const FVector ViewPoint = CandidatePosition->GetActorLocation() + FVector::UpVector * 100.0f;
	if (const AController* Controller = Cast<AController>(PositioningComponent->GetOwner()))
	{
		return Controller->LineOfSightTo(TargetActor, ViewPoint, true) ? FMath::FRand() : 0.0f;
	}

	const UWorld* World = CandidatePosition->GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RandomLineOfSightFiringPositionEval), true);
	QueryParams.AddIgnoredActor(CandidatePosition);
	const bool bHit = World->LineTraceSingleByObjectType(
		HitResult,
		ViewPoint,
		TargetActor->GetActorLocation(),
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		QueryParams);

	return (!bHit || HitResult.GetActor() == TargetActor) ? FMath::FRand() : 0.0f;
}

float URandomPatrolFiringPositionEval::EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const
{
	return (PositioningComponent && CandidatePosition) ? FMath::FRand() : 0.0f;
}
