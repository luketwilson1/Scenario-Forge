// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentCustomization.h
 * @brief Declares the data used to customize an agent's actions and weapons.
 */

#pragma once

#include "CoreMinimal.h"
#include "CustomizationTypes.h"
#include "Engine/DataAsset.h"
#include "FactionTypes.h"
#include "GameplayTagContainer.h"
#include "AgentCustomization.generated.h"

class UActionDefinition;
class AWeapon;


/**
 * @brief Defines the configurable sensory ranges and field of view for an agent.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FPerception
{
	GENERATED_BODY()

public:

	/** Maximum sight distance, in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float SightRadius = 0.0f;

	/** Distance, in centimeters, at which a visible target is considered lost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float LoseSightRadius = 0.0f;

	/** Peripheral vision angle, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Degrees", ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
	float PeripheralVisionAngleDegrees = 0.0f;

	/** Maximum hearing distance, in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float HearingRange = 0.0f;
};

/**
 * @brief Defines the configurable firing ranges for a weapon.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FWeaponProperties
{
	GENERATED_BODY()

public:

	/** Maximum distance, in centimeters, at which the weapon may fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float MaximumFiringRange = 0.0f;

	/** Minimum distance, in centimeters, at which the weapon may fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float MinimumFiringRange = 0.0f;
};

/**
 * @brief Stores the actions and per-weapon-class properties available to an agent.
 */
UCLASS()
class SCENARIOFORGE_API UAgentCustomization : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Actions available to the agent, with duplicate entries disallowed in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions", meta = (NoElementDuplicate))
	TArray<TObjectPtr<UActionDefinition>> Actions;

	/** Goal tags assigned to the agent's decision component when the AI controller possesses it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goals")
	FGameplayTagContainer StartingGoalTags;

	/** Visual presentation used by the agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAppearance Appearance;

	/** Faction this agent belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent")
	EFaction Faction = EFaction::Red;

	/** Sensory ranges and field of view used by the agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FPerception Perception;

	/** Properties associated with each weapon class derived from AWeapon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	TMap<TSubclassOf<AWeapon>, FWeaponProperties> WeaponProperties;


};
