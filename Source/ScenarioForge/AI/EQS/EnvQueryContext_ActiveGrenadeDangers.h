// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file EnvQueryContext_ActiveGrenadeDangers.h
 * @brief Declares an EQS context that exposes explosive projectiles threatening the querier.
 */

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_ActiveGrenadeDangers.generated.h"

/** @brief Supplies every active grenade danger source currently overlapping the querying agent. */
UCLASS()
class SCENARIOFORGE_API UEnvQueryContext_ActiveGrenadeDangers : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	/** Provides active explosive actors tracked by the querying agent. */
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
