// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Scenario.h
 * @brief Declares the authored scenario root actor.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Scenario.generated.h"

class AObjective;
class ASquad;
class ASquadGroup;
class AZone;

/**
 * @brief Root actor for authored scenario data.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API AScenario : public AActor
{
	GENERATED_BODY()

public:
	/** Initializes this scenario actor. */
	AScenario();

	/** Squad groups authored for this scenario. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (NoElementDuplicate))
	TArray<TObjectPtr<ASquadGroup>> SquadGroups;

	/** Squads authored for this scenario. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (NoElementDuplicate))
	TArray<TObjectPtr<ASquad>> Squads;

	/** Tactical zones authored for this scenario. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (NoElementDuplicate))
	TArray<TObjectPtr<AZone>> Zones;

	/** Objectives authored for this scenario. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (NoElementDuplicate))
	TArray<TObjectPtr<AObjective>> Objectives;
};
