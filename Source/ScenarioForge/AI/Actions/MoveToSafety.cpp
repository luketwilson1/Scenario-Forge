// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file MoveToSafety.cpp
 * @brief Implements EQS-driven movement outside active grenade explosion radii.
 */

#include "MoveToSafety.h"

#include "Agent.h"
#include "AgentAIController.h"
#include "AgentCustomization.h"
#include "Components/CapsuleComponent.h"
#include "EQS/EnvQueryContext_ActiveGrenadeDangers.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_SimpleGrid.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Distance.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Pathfinding.h"
#include "Navigation/PathFollowingComponent.h"
#include "Planner.h"
#include "Projectile.h"
#include "ScenarioForgeGameplayTags.h"

UMoveToSafety::UMoveToSafety()
{
	TruePreconditions.AddTag(TAG_State_GrenadeNear.GetTag());
	FalsePreconditions.AddTag(TAG_State_Dead.GetTag());
	AddedEffects.AddTag(TAG_State_SelfPreserve.GetTag());
	RemovedEffects.AddTag(TAG_State_GrenadeNear.GetTag());
}

EActionResult UMoveToSafety::Execute(UPlanner* Planner)
{
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	AAgent* Agent = Controller ? Cast<AAgent>(Controller->GetPawn()) : nullptr;
	const UAgentCustomization* Customization = Agent ? Agent->GetAgentCustomization() : nullptr;
	const FDodgeProperties* DodgeProperties = Customization ? &Customization->GetResolvedDodgeProperties() : nullptr;
	if (!Planner || !Controller || !Agent || !DodgeProperties)
	{
		return EActionResult::Failed;
	}

	TArray<AActor*> DangerSources;
	Agent->GetGrenadeDangerSources(DangerSources);
	if (DangerSources.IsEmpty())
	{
		return EActionResult::Succeeded;
	}

	float LargestOuterRadius = 0.0f;
	for (AActor* DangerSource : DangerSources)
	{
		if (const AProjectile* Projectile = Cast<AProjectile>(DangerSource))
		{
			LargestOuterRadius = FMath::Max(LargestOuterRadius, Projectile->GetDetonationOuterRadius());
		}
	}

	const float CapsuleRadius = Agent->GetCapsuleComponent() ? Agent->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.0f;
	const float SafeDistance = LargestOuterRadius + FMath::Max(0.0f, DodgeProperties->SafetyMargin) + CapsuleRadius;
	const float GridHalfSize = FMath::Max(SafeDistance, DodgeProperties->SafetyGridHalfSize);
	RuntimeSafetyQuery = BuildSafetyQuery(SafeDistance, GridHalfSize);
	if (!RuntimeSafetyQuery)
	{
		return EActionResult::Failed;
	}

	ExecutingPlanner = Planner;
	FEnvQueryRequest QueryRequest(RuntimeSafetyQuery, Agent);
	QueryID = QueryRequest.Execute(EEnvQueryRunMode::SingleResult, this, &UMoveToSafety::HandleQueryFinished);
	if (QueryID == INDEX_NONE)
	{
		ExecutingPlanner.Reset();
		return EActionResult::Failed;
	}

	return EActionResult::Running;
}

UEnvQuery* UMoveToSafety::BuildSafetyQuery(float SafeDistance, float GridHalfSize)
{
	UEnvQuery* Query = NewObject<UEnvQuery>(this, TEXT("RuntimeMoveToSafetyQuery"));
	UEnvQueryOption* Option = Query ? NewObject<UEnvQueryOption>(Query) : nullptr;
	if (!Query || !Option)
	{
		return nullptr;
	}

	UEnvQueryGenerator_SimpleGrid* Grid = NewObject<UEnvQueryGenerator_SimpleGrid>(Option);
	UEnvQueryTest_Pathfinding* PathTest = NewObject<UEnvQueryTest_Pathfinding>(Option);
	UEnvQueryTest_Distance* OutsideDangerTest = NewObject<UEnvQueryTest_Distance>(Option);
	UEnvQueryTest_Distance* ClosestPointTest = NewObject<UEnvQueryTest_Distance>(Option);
	if (!Grid || !PathTest || !OutsideDangerTest || !ClosestPointTest)
	{
		return nullptr;
	}

	Grid->GenerateAround = UEnvQueryContext_Querier::StaticClass();
	Grid->GridSize.DefaultValue = FMath::Max(100.0f, GridHalfSize);
	Grid->SpaceBetween.DefaultValue = 150.0f;
	Option->Generator = Grid;

	PathTest->TestMode = EEnvTestPathfinding::PathExist;
	PathTest->Context = UEnvQueryContext_Querier::StaticClass();
	PathTest->PathFromContext.DefaultValue = true;
	PathTest->TestPurpose = EEnvTestPurpose::Filter;
	PathTest->FilterType = EEnvTestFilterType::Match;
	PathTest->BoolValue.DefaultValue = true;
	PathTest->TestOrder = 0;

	OutsideDangerTest->TestMode = EEnvTestDistance::Distance2D;
	OutsideDangerTest->DistanceTo = UEnvQueryContext_ActiveGrenadeDangers::StaticClass();
	OutsideDangerTest->TestPurpose = EEnvTestPurpose::Filter;
	OutsideDangerTest->FilterType = EEnvTestFilterType::Minimum;
	OutsideDangerTest->FloatValueMin.DefaultValue = FMath::Max(0.0f, SafeDistance);
	OutsideDangerTest->MultipleContextFilterOp = EEnvTestFilterOperator::AllPass;
	OutsideDangerTest->TestOrder = 1;

	ClosestPointTest->TestMode = EEnvTestDistance::Distance2D;
	ClosestPointTest->DistanceTo = UEnvQueryContext_Querier::StaticClass();
	ClosestPointTest->TestPurpose = EEnvTestPurpose::Score;
	ClosestPointTest->ScoringEquation = EEnvTestScoreEquation::InverseLinear;
	ClosestPointTest->ScoringFactor.DefaultValue = 1.0f;
	ClosestPointTest->TestOrder = 2;

	Option->Tests.Add(PathTest);
	Option->Tests.Add(OutsideDangerTest);
	Option->Tests.Add(ClosestPointTest);
	Query->GetOptionsMutable().Add(Option);
	return Query;
}

bool UMoveToSafety::Interrupt(UPlanner* Planner)
{
	if (!ExecutingPlanner.IsValid() || ExecutingPlanner.Get() != Planner)
	{
		return false;
	}

	Cleanup(true);
	ExecutingPlanner.Reset();
	return true;
}

void UMoveToSafety::BeginDestroy()
{
	Cleanup(false);
	Super::BeginDestroy();
}

void UMoveToSafety::HandleQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	QueryID = INDEX_NONE;
	UPlanner* Planner = ExecutingPlanner.Get();
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	UPathFollowingComponent* PathFollowing = Controller ? Controller->GetPathFollowingComponent() : nullptr;
	if (!Planner || !Controller || !PathFollowing || !Result.IsValid() || !Result->IsSuccessful() || Result->Items.IsEmpty())
	{
		Cleanup(false);
		ExecutingPlanner.Reset();
		if (Planner)
		{
			Planner->CompleteActiveAction(EActionResult::Failed);
		}
		return;
	}

	MoveCompletedDelegateHandle = PathFollowing->OnRequestFinished.AddUObject(this, &UMoveToSafety::HandleMoveCompleted);
	const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(Result->GetItemAsLocation(0));
	MoveRequestID = Controller->GetCurrentMoveRequestID();
	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		Cleanup(false);
		ExecutingPlanner.Reset();
		Planner->CompleteActiveAction(EActionResult::Failed);
	}
	else if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		HandleMoveCompleted(MoveRequestID, FPathFollowingResult(EPathFollowingResult::Success));
	}
}

void UMoveToSafety::HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (!MoveRequestID.IsValid() || !RequestID.IsEquivalent(MoveRequestID))
	{
		return;
	}

	UPlanner* Planner = ExecutingPlanner.Get();
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	AAgent* Agent = Controller ? Cast<AAgent>(Controller->GetPawn()) : nullptr;
	const bool bReachedSafety = Result.IsSuccess() && Agent && !Agent->HasGrenadeDangerSources();
	Cleanup(false);
	ExecutingPlanner.Reset();
	if (Planner)
	{
		Planner->CompleteActiveAction(bReachedSafety ? EActionResult::Succeeded : EActionResult::Failed);
	}
}

void UMoveToSafety::Cleanup(bool bStopMovement)
{
	UPlanner* Planner = ExecutingPlanner.Get();
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	if (QueryID != INDEX_NONE)
	{
		if (UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(Planner))
		{
			QueryManager->AbortQuery(QueryID);
		}
		QueryID = INDEX_NONE;
	}

	if (Controller)
	{
		if (UPathFollowingComponent* PathFollowing = Controller->GetPathFollowingComponent())
		{
			PathFollowing->OnRequestFinished.Remove(MoveCompletedDelegateHandle);
		}
		if (bStopMovement)
		{
			Controller->StopMovement();
		}
	}

	MoveCompletedDelegateHandle.Reset();
	MoveRequestID = FAIRequestID::InvalidRequest;
	RuntimeSafetyQuery = nullptr;
}
