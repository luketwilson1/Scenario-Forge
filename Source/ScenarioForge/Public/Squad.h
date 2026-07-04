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
 * @brief Squad-level knowledge about an enemy actor.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FKnownEnemyTarget
{
	GENERATED_BODY()

public:
	/** Enemy actor this record describes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<AActor> Actor;

	/** Last position reported for this enemy by squad perception. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	FVector LastKnownLocation = FVector::ZeroVector;

	/** World time when LastKnownLocation was last updated. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	float LastKnownTime = 0.0f;

	/** True while at least one assigned squad agent currently sees this enemy. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	bool bCurrentlySeen = false;

	/** True when this enemy is not currently seen but at least one assigned squad agent remembers it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	bool bRemembered = false;
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

	/** Adds an enemy actor currently perceived by at least one squad agent. */
	void AddPerceivedEnemy(AActor* EnemyActor);

	/** Removes an enemy actor only after no squad agent still sees it. */
	void RemovePerceivedEnemyIfUnseen(AActor* EnemyActor);

	/** Checks whether any assigned squad agent currently sees the supplied enemy actor. */
	bool IsEnemySeenByAnyAgent(const AActor* EnemyActor) const;

	/** Adds an enemy actor currently remembered by at least one squad agent. */
	void AddRememberedTarget(AActor* EnemyActor);

	/** Removes an enemy actor only after no squad agent still remembers it. */
	void RemoveRememberedTargetIfUnremembered(AActor* EnemyActor);

	/** Checks whether any assigned squad agent currently remembers the supplied enemy actor. */
	bool IsEnemyRememberedByAnyAgent(const AActor* EnemyActor) const;

	/** Gets the known target record for an enemy actor, or nullptr when this squad has no record. */
	const FKnownEnemyTarget* FindKnownEnemyTarget(const AActor* EnemyActor) const;

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

	/** Enemy target knowledge shared by this squad. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TArray<FKnownEnemyTarget> KnownEnemyTargets;

private:
	/** Gets or creates the known target record for an enemy actor. */
	FKnownEnemyTarget* FindOrAddKnownEnemyTarget(AActor* EnemyActor);

	/** Gets the mutable known target record for an enemy actor, or nullptr when missing. */
	FKnownEnemyTarget* FindKnownEnemyTargetMutable(const AActor* EnemyActor);

	/** Refreshes actor, location, and timestamp fields on a known target record. */
	void UpdateKnownEnemyTargetLocation(FKnownEnemyTarget& KnownEnemyTarget, AActor* EnemyActor);
};
