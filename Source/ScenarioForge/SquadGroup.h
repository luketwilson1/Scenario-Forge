// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file SquadGroup.h
 * @brief Declares authored squad groups used to organize AI assignments.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SquadGroup.generated.h"

class AObjective;

/**
 * @brief Designer-authored squad group with hierarchy and objective assignment.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API ASquadGroup : public AActor
{
	GENERATED_BODY()

public:
	/** Initializes this squad group actor. */
	ASquadGroup();

	/** Designer-facing squad group label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString SquadGroupName;

	/** Parent squad group in the authored command hierarchy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<ASquadGroup> Parent;

	/** Objective assigned to this squad group. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<AObjective> Objective;
};
