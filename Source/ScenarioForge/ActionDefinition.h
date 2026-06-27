// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ActionDefinition.h
 * @brief Declares the data asset that describes planner-visible GOAP actions.
 */

#pragma once

#include "CoreMinimal.h"
#include "ActionBehavior.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ActionDefinition.generated.h"

/**
 * @brief Defines the tags and behavior class for a GOAP action.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API UActionDefinition : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Behavior class used to execute this action when it is selected by a plan. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	TSubclassOf<UActionBehavior> BehaviorClass;

	/** Tags that must be present before this action can be used by the planner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTagContainer RequiredTags;

	/** Tags that prevent this action from being used by the planner when present. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTagContainer BlockedTags;

	/** Tags added to the simulated state after this action is applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTagContainer AddedTags;

	/** Tags removed from the simulated state after this action is applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
	FGameplayTagContainer RemovedTags;
};
