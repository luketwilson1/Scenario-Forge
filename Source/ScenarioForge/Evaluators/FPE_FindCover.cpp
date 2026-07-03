// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FPE_FindCover.cpp
 * @brief Implements the cover firing-position evaluator.
 */

#include "FPE_FindCover.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "../FiringPosition.h"
#include "../TacticalPositioningComponent.h"

float UFPE_FindCover::EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const
{
	if (!PositioningComponent || !CandidatePosition)
	{
		return 0.0f;
	}

	const int32 Flags = CandidatePosition->PositionFlags;
	if ((Flags & static_cast<int32>(EFiringPositionFlag::Perch)) != 0)
	{
		return 0.0f;
	}

	float CoverScore = 0.0f;
	if ((Flags & static_cast<int32>(EFiringPositionFlag::Partial)) != 0)
	{
		CoverScore = FMath::Max(CoverScore, PartialCoverScore);
	}
	if ((Flags & static_cast<int32>(EFiringPositionFlag::Closed)) != 0)
	{
		CoverScore = FMath::Max(CoverScore, ClosedCoverScore);
	}
	if ((Flags & static_cast<int32>(EFiringPositionFlag::WallLean)) != 0)
	{
		CoverScore = FMath::Max(CoverScore, WallLeanScore);
	}

	if (CoverScore <= 0.0f || !bRequireBlockedLineOfSightFromCurrentTarget)
	{
		return CoverScore;
	}

	const AActor* TargetActor = PositioningComponent->GetTargetActor();
	const UWorld* World = CandidatePosition->GetWorld();
	if (!TargetActor || !World)
	{
		return 0.0f;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FPE_FindCover), true);
	QueryParams.AddIgnoredActor(TargetActor);
	QueryParams.AddIgnoredActor(CandidatePosition);
	QueryParams.AddIgnoredActor(PositioningComponent->GetOwner());

	const FVector CandidateViewPoint = CandidatePosition->GetActorLocation() + FVector::UpVector * CoverCheckEyeHeight;
	const bool bHit = World->LineTraceSingleByObjectType(
		HitResult,
		TargetActor->GetActorLocation(),
		CandidateViewPoint,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		QueryParams);

	return (bHit && HitResult.GetActor() != CandidatePosition) ? CoverScore : 0.0f;
}
