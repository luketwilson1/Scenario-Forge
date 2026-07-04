// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FiringPositionEval.cpp
 * @brief Implements the default firing position evaluator behavior.
 */

#include "FiringPositionEval.h"

#include "../AgentAIController.h"
#include "../AgentCustomization.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "../FiringPosition.h"
#include "GameFramework/Controller.h"
#include "EngineUtils.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "../Public/Agent.h"
#include "../Public/Squad.h"
#include "../TacticalPositioningComponent.h"
#include "../Weapon.h"
#include "../WeaponCustomization.h"

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

float UFiringPositionEval::EvaluateRawScoreWithDebug(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition, bool bDebugDraw) const
{
	return EvaluateRawScore(PositioningComponent, CandidatePosition);
}

float UFPE_LineOfSight::EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const
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
		return Controller->LineOfSightTo(TargetActor, ViewPoint, true) ? 1.0f : 0.0f;
	}

	const UWorld* World = CandidatePosition->GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FPE_LineOfSight), true);
	QueryParams.AddIgnoredActor(CandidatePosition);
	const bool bHit = World->LineTraceSingleByObjectType(
		HitResult,
		ViewPoint,
		TargetActor->GetActorLocation(),
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		QueryParams);

	return (!bHit || HitResult.GetActor() == TargetActor) ? 1.0f : 0.0f;
}

float UFPE_WeaponRange::EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const
{
	if (!PositioningComponent || !CandidatePosition)
	{
		return 0.0f;
	}

	const AActor* TargetActor = PositioningComponent->GetTargetActor();
	if (!TargetActor)
	{
		return 0.0f;
	}

	const AAgentAIController* AgentAIController = Cast<AAgentAIController>(PositioningComponent->GetOwner());
	const AAgent* Agent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	const UAgentCustomization* AgentCustomization = Agent ? Agent->GetAgentCustomization() : nullptr;
	const AWeapon* PrimaryWeapon = Agent ? Agent->GetPrimaryWeapon() : nullptr;
	const UWeaponCustomization* WeaponCustomization = PrimaryWeapon ? PrimaryWeapon->GetActiveWeaponCustomization() : nullptr;
	const FWeaponProperties* WeaponProperties = AgentCustomization && WeaponCustomization
		? AgentCustomization->FindResolvedWeaponProperties(WeaponCustomization)
		: nullptr;
	if (!WeaponProperties || WeaponProperties->MaximumFiringRange <= 0.0f)
	{
		return 0.0f;
	}

	const float DistanceSquared = FVector::DistSquared(CandidatePosition->GetActorLocation(), TargetActor->GetActorLocation());
	if (WeaponProperties->MinimumFiringRange > 0.0f && DistanceSquared < FMath::Square(WeaponProperties->MinimumFiringRange))
	{
		return 0.0f;
	}

	return DistanceSquared <= FMath::Square(WeaponProperties->MaximumFiringRange) ? 1.0f : 0.0f;
}

float UFPE_Reachable::EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const
{
	if (!PositioningComponent || !CandidatePosition)
	{
		return 0.0f;
	}

	const AAgentAIController* AgentAIController = Cast<AAgentAIController>(PositioningComponent->GetOwner());
	APawn* ControlledPawn = AgentAIController ? AgentAIController->GetPawn() : nullptr;
	UWorld* World = CandidatePosition->GetWorld();
	if (!ControlledPawn || !World)
	{
		return 0.0f;
	}

	const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
		World,
		ControlledPawn->GetActorLocation(),
		CandidatePosition->GetActorLocation(),
		ControlledPawn);

	return (Path && Path->IsValid() && !Path->IsPartial()) ? 1.0f : 0.0f;
}

float UFPE_LastKnownLOS::EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const
{
	return EvaluateRawScoreWithDebug(PositioningComponent, CandidatePosition, false);
}

float UFPE_LastKnownLOS::EvaluateRawScoreWithDebug(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition, bool bDebugDraw) const
{
	if (!PositioningComponent || !CandidatePosition)
	{
		return 0.0f;
	}

	const AAgentAIController* AgentAIController = Cast<AAgentAIController>(PositioningComponent->GetOwner());
	const AAgent* Agent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	UWorld* World = CandidatePosition->GetWorld();
	if (!Agent || !World)
	{
		return 0.0f;
	}

	const ASquad* OwningSquad = nullptr;
	for (TActorIterator<ASquad> It(World); It; ++It)
	{
		const ASquad* Squad = *It;
		if (IsValid(Squad) && Squad->Agents.Contains(Agent))
		{
			OwningSquad = Squad;
			break;
		}
	}

	if (!OwningSquad)
	{
		return 0.0f;
	}

	const FKnownEnemyTarget* SelectedTarget = nullptr;
	for (const FKnownEnemyTarget& KnownEnemyTarget : OwningSquad->KnownEnemyTargets)
	{
		if (!KnownEnemyTarget.bRemembered || KnownEnemyTarget.bCurrentlySeen)
		{
			continue;
		}

		if (!SelectedTarget || KnownEnemyTarget.LastKnownTime > SelectedTarget->LastKnownTime)
		{
			SelectedTarget = &KnownEnemyTarget;
		}
	}

	if (!SelectedTarget)
	{
		return 0.0f;
	}

	const FVector ViewPoint = CandidatePosition->GetActorLocation() + FVector::UpVector * 100.0f;
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FPE_LastKnownLOS), true);
	QueryParams.AddIgnoredActor(CandidatePosition);
	QueryParams.AddIgnoredActor(Agent);
	const bool bHit = World->LineTraceSingleByObjectType(
		HitResult,
		ViewPoint,
		SelectedTarget->LastKnownLocation,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		QueryParams);

	if (bDebugDraw)
	{
		const bool bHasLineOfSight = !bHit;
		const FColor TraceColor = bHasLineOfSight ? FColor::Green : FColor::Red;
		DrawDebugLine(World, ViewPoint, SelectedTarget->LastKnownLocation, TraceColor, false, 1.0f, 0, 3.0f);
		DrawDebugSphere(World, SelectedTarget->LastKnownLocation, 25.0f, 12, FColor::Cyan, false, 1.0f, 0, 2.0f);

		if (bHit)
		{
			DrawDebugPoint(World, HitResult.ImpactPoint, 12.0f, FColor::Yellow, false, 1.0f, 0);
		}
	}

	return bHit ? 0.0f : 1.0f;
}

float URandomPatrolFiringPositionEval::EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const
{
	return (PositioningComponent && CandidatePosition) ? FMath::FRand() : 0.0f;
}
