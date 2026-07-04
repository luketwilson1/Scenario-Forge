// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Squad.cpp
 * @brief Implements authored squads.
 */

#include "Squad.h"

#include "Agent.h"
#include "AgentAIController.h"

ASquad::ASquad()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASquad::AddPerceivedEnemy(AActor* EnemyActor)
{
	if (!IsValid(EnemyActor))
	{
		return;
	}

	FKnownEnemyTarget* KnownEnemyTarget = FindOrAddKnownEnemyTarget(EnemyActor);
	if (!KnownEnemyTarget)
	{
		return;
	}

	UpdateKnownEnemyTargetLocation(*KnownEnemyTarget, EnemyActor);
	KnownEnemyTarget->bCurrentlySeen = true;
	KnownEnemyTarget->bRemembered = false;
}

void ASquad::RemovePerceivedEnemyIfUnseen(AActor* EnemyActor)
{
	FKnownEnemyTarget* KnownEnemyTarget = FindKnownEnemyTargetMutable(EnemyActor);
	if (!KnownEnemyTarget)
	{
		return;
	}

	if (!IsValid(EnemyActor))
	{
		KnownEnemyTargets.RemoveAll([EnemyActor](const FKnownEnemyTarget& KnownEnemy)
		{
			return KnownEnemy.Actor.Get() == EnemyActor || !IsValid(KnownEnemy.Actor);
		});
		return;
	}

	if (IsEnemySeenByAnyAgent(EnemyActor))
	{
		UpdateKnownEnemyTargetLocation(*KnownEnemyTarget, EnemyActor);
		KnownEnemyTarget->bCurrentlySeen = true;
		KnownEnemyTarget->bRemembered = false;
		return;
	}

	KnownEnemyTarget->bCurrentlySeen = false;
	KnownEnemyTarget->bRemembered = IsEnemyRememberedByAnyAgent(EnemyActor);
	if (!KnownEnemyTarget->bRemembered)
	{
		KnownEnemyTargets.RemoveAll([EnemyActor](const FKnownEnemyTarget& KnownEnemy)
		{
			return KnownEnemy.Actor.Get() == EnemyActor || !IsValid(KnownEnemy.Actor);
		});
	}
}

bool ASquad::IsEnemySeenByAnyAgent(const AActor* EnemyActor) const
{
	if (!IsValid(EnemyActor))
	{
		return false;
	}

	for (const AAgent* Agent : Agents)
	{
		const AAgentAIController* AgentAIController = Agent ? Cast<AAgentAIController>(Agent->GetController()) : nullptr;
		if (AgentAIController && AgentAIController->IsSeeingEnemyActor(EnemyActor))
		{
			return true;
		}
	}

	return false;
}

void ASquad::AddRememberedTarget(AActor* EnemyActor)
{
	if (!IsValid(EnemyActor))
	{
		return;
	}

	if (IsEnemySeenByAnyAgent(EnemyActor))
	{
		AddPerceivedEnemy(EnemyActor);
		return;
	}

	FKnownEnemyTarget* KnownEnemyTarget = FindOrAddKnownEnemyTarget(EnemyActor);
	if (!KnownEnemyTarget)
	{
		return;
	}

	UpdateKnownEnemyTargetLocation(*KnownEnemyTarget, EnemyActor);
	KnownEnemyTarget->bCurrentlySeen = false;
	KnownEnemyTarget->bRemembered = true;
}

void ASquad::RemoveRememberedTargetIfUnremembered(AActor* EnemyActor)
{
	FKnownEnemyTarget* KnownEnemyTarget = FindKnownEnemyTargetMutable(EnemyActor);
	if (!KnownEnemyTarget)
	{
		return;
	}

	if (!IsValid(EnemyActor))
	{
		KnownEnemyTargets.RemoveAll([EnemyActor](const FKnownEnemyTarget& KnownEnemy)
		{
			return KnownEnemy.Actor.Get() == EnemyActor || !IsValid(KnownEnemy.Actor);
		});
		return;
	}

	if (IsEnemySeenByAnyAgent(EnemyActor))
	{
		UpdateKnownEnemyTargetLocation(*KnownEnemyTarget, EnemyActor);
		KnownEnemyTarget->bCurrentlySeen = true;
		KnownEnemyTarget->bRemembered = false;
		return;
	}

	KnownEnemyTarget->bRemembered = IsEnemyRememberedByAnyAgent(EnemyActor);
	if (!KnownEnemyTarget->bRemembered)
	{
		KnownEnemyTargets.RemoveAll([EnemyActor](const FKnownEnemyTarget& KnownEnemy)
		{
			return KnownEnemy.Actor.Get() == EnemyActor || !IsValid(KnownEnemy.Actor);
		});
	}
}

bool ASquad::IsEnemyRememberedByAnyAgent(const AActor* EnemyActor) const
{
	if (!IsValid(EnemyActor))
	{
		return false;
	}

	for (const AAgent* Agent : Agents)
	{
		const AAgentAIController* AgentAIController = Agent ? Cast<AAgentAIController>(Agent->GetController()) : nullptr;
		if (AgentAIController && AgentAIController->IsRememberingEnemyActor(EnemyActor))
		{
			return true;
		}
	}

	return false;
}

const FKnownEnemyTarget* ASquad::FindKnownEnemyTarget(const AActor* EnemyActor) const
{
	if (!EnemyActor)
	{
		return nullptr;
	}

	return KnownEnemyTargets.FindByPredicate([EnemyActor](const FKnownEnemyTarget& KnownEnemy)
	{
		return KnownEnemy.Actor.Get() == EnemyActor;
	});
}

FKnownEnemyTarget* ASquad::FindOrAddKnownEnemyTarget(AActor* EnemyActor)
{
	if (!IsValid(EnemyActor))
	{
		return nullptr;
	}

	KnownEnemyTargets.RemoveAll([](const FKnownEnemyTarget& KnownEnemy)
	{
		return !IsValid(KnownEnemy.Actor);
	});

	if (FKnownEnemyTarget* ExistingTarget = FindKnownEnemyTargetMutable(EnemyActor))
	{
		return ExistingTarget;
	}

	FKnownEnemyTarget& NewTarget = KnownEnemyTargets.AddDefaulted_GetRef();
	NewTarget.Actor = EnemyActor;
	return &NewTarget;
}

FKnownEnemyTarget* ASquad::FindKnownEnemyTargetMutable(const AActor* EnemyActor)
{
	if (!EnemyActor)
	{
		return nullptr;
	}

	return KnownEnemyTargets.FindByPredicate([EnemyActor](const FKnownEnemyTarget& KnownEnemy)
	{
		return KnownEnemy.Actor.Get() == EnemyActor;
	});
}

void ASquad::UpdateKnownEnemyTargetLocation(FKnownEnemyTarget& KnownEnemyTarget, AActor* EnemyActor)
{
	if (!IsValid(EnemyActor))
	{
		return;
	}

	KnownEnemyTarget.Actor = EnemyActor;
	KnownEnemyTarget.LastKnownLocation = EnemyActor->GetActorLocation();

	const UWorld* World = GetWorld();
	KnownEnemyTarget.LastKnownTime = World ? World->GetTimeSeconds() : 0.0f;
}
