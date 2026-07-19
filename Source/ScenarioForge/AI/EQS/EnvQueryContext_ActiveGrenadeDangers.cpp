// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file EnvQueryContext_ActiveGrenadeDangers.cpp
 * @brief Implements the active grenade danger EQS context.
 */

#include "EnvQueryContext_ActiveGrenadeDangers.h"

#include "Agent.h"
#include "AIController.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

void UEnvQueryContext_ActiveGrenadeDangers::ProvideContext(
	FEnvQueryInstance& QueryInstance,
	FEnvQueryContextData& ContextData) const
{
	AAgent* Agent = Cast<AAgent>(QueryInstance.Owner.Get());
	if (!Agent)
	{
		const AAIController* Controller = Cast<AAIController>(QueryInstance.Owner.Get());
		Agent = Controller ? Cast<AAgent>(Controller->GetPawn()) : nullptr;
	}

	if (!Agent)
	{
		return;
	}

	TArray<AActor*> DangerSources;
	Agent->GetGrenadeDangerSources(DangerSources);
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, DangerSources);
}
