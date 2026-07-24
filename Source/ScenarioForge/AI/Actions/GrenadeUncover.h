// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GrenadeUncover.h
 * @brief Declares the grenade action used to flush a stationary enemy from cover.
 */

#pragma once

#include "CoreMinimal.h"
#include "ThrowGrenade.h"
#include "GrenadeUncover.generated.h"

/** Throws a grenade at the last observed position of a qualifying stationary enemy. */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UGrenadeUncover : public UThrowGrenade
{
	GENERATED_BODY()

public:
	/** Uses stationary-target and grenade-readiness states for planning. */
	UGrenadeUncover();

protected:
	/** Retrieves the stationary enemy's last-observed location. */
	virtual bool GetCachedTargetLocation(const AAgentAIController& Controller, FVector& OutTargetLocation) const override;

	/** Retrieves the trajectory cached specifically for the stationary enemy. */
	virtual bool GetCachedThrowSolution(const AAgentAIController& Controller, FGrenadeThrowSolution& OutSolution) const override;
};
