// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Squad.h
 * @brief Declares authored squads used by scenarios.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../FactionTypes.h"
#include "Squad.generated.h"

class AAgent;
class AObjective;
class ASquadGroup;
class AZone;

/**
 * @brief Authored behavior and sensory flags for a squad.
 */
UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ESquadFlag : uint8
{
	None = 0 UMETA(Hidden),
	Blind = 1 << 0 UMETA(DisplayName = "Blind"),
	Deaf = 1 << 1 UMETA(DisplayName = "Deaf"),
	Braindead = 1 << 2 UMETA(DisplayName = "Braindead"),
	InTeamDisguise = 1 << 3 UMETA(DisplayName = "In Team Disguise")
};

/**
 * @brief Designer-authored squad.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API ASquad : public AActor
{
	GENERATED_BODY()

public:
	/** Initializes this squad actor. */
	ASquad();

	/** Designer-facing squad label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString SquadName;

	/** Behavior and sensory flags for this squad. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Bitmask, BitmaskEnum = "/Script/ScenarioForge.ESquadFlag"))
	int32 Flags = static_cast<int32>(ESquadFlag::None);

	/** Faction/team this squad belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EFaction Team = EFaction::None;

	/** Squad group in the authored command hierarchy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<ASquadGroup> SquadGroup;

	/** Objective assigned to this squad. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<AObjective> Objective;

	/** Tactical zone containing firing positions this squad can use. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<AZone> Zone;

	/** Agents assigned to this squad. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (NoElementDuplicate))
	TArray<TObjectPtr<AAgent>> Agents;
};
