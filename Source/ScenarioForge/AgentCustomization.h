// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentCustomization.h
 * @brief Declares the data used to customize an agent's actions and weapons.
 */

#pragma once

#include "CoreMinimal.h"
#include "CustomizationTypes.h"
#include "Engine/DataAsset.h"
#include "TacticalPositioningComponent.h"
#include "FactionTypes.h"
#include "GameplayTagContainer.h"
#include "AgentCustomization.generated.h"

class UActionDefinition;
class UFiringPositionEval;
class UWeaponCustomization;


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
 * @brief Defines the configurable AI firing preferences for a weapon.
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

	/** Minimum time, in seconds, an AI burst should last when using this weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0"))
	float MinimumBurstDuration = 0.0f;

	/** Maximum time, in seconds, an AI burst should last when using this weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0"))
	float MaximumBurstDuration = 0.0f;

	/** Minimum time, in seconds, the AI should wait after firing a burst with this weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0"))
	float MinimumBurstSeparation = 0.0f;

	/** Maximum time, in seconds, the AI should wait after firing a burst with this weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0"))
	float MaximumBurstSeparation = 0.0f;
};

/**
 * @brief Defines engagement timing preferences for combat movement decisions.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FEngageProperties
{
	GENERATED_BODY()

public:

	/** Minimum time, in seconds, before the agent considers repositioning while engaged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Minimum Reposition Delay"))
	float MinimumRepositionDelay = 0.0f;

	/** Maximum time, in seconds, before the agent considers repositioning while engaged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Maximum Reposition Delay"))
	float MaximumRepositionDelay = 0.0f;

	/** Distance, in centimeters, from a reposition destination that counts as arrival. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Reposition Acceptance Radius"))
	float RepositionAcceptanceRadius = 150.0f;
};

/**
 * @brief Defines cover timing preferences for defensive movement decisions.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FCoverProperties
{
	GENERATED_BODY()

public:

	/** Minimum time, in seconds, to remain behind cover before reconsidering movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Minimum Hide Behind Cover Time"))
	float MinimumHideBehindCoverTime = 0.0f;

	/** Maximum time, in seconds, to remain behind cover before reconsidering movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Maximum Hide Behind Cover Time"))
	float MaximumHideBehindCoverTime = 0.0f;

	/** Shield percentage below which the agent should consider taking cover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Percent", ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", DisplayName = "Cover Shield Fraction"))
	float CoverShieldFraction = 0.0f;

	/** Vitality percentage below which the agent should consider taking cover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Percent", ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", DisplayName = "Cover Vitality Threshold"))
	float CoverVitalityThreshold = 0.0f;

	/** Danger threshold above which the agent should consider taking cover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0", DisplayName = "Cover Danger Threshold"))
	float CoverDangerThreshold = 0.0f;
};

/**
 * @brief Defines health values used to initialize an agent's vitality attributes.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FVitalityProperties
{
	GENERATED_BODY()

public:

	/** Maximum health assigned to the agent at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHealth = 100.0f;
};

/**
 * @brief Designer-authored scoring configuration for one firing position evaluator.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FTacticalPositionEvaluatorConfig
{
	GENERATED_BODY()

public:

	/** Evaluator used to produce the raw value that drives the score curve. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluator")
	TSubclassOf<UFiringPositionEval> EvaluatorClass;

	/** Points awarded when the score curve evaluates to 1. Negative values make the evaluator a penalty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluator")
	float Points = 0.0f;

};

/**
 * @brief Firing position evaluator set used by a tactical movement mode.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FTacticalMovementModeEvaluatorSet
{
	GENERATED_BODY()

public:

	/** Evaluators run when this movement mode scores firing positions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluator", meta = (NoElementDuplicate))
	TArray<FTacticalPositionEvaluatorConfig> Evaluators;
};

/**
 * @brief Stores the actions and per-weapon-customization properties available to an agent.
 */
UCLASS()
class SCENARIOFORGE_API UAgentCustomization : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Parent sheet used as a fallback for sections this sheet does not override. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inheritance", meta = (DisplayPriority = "0"))
	TObjectPtr<UAgentCustomization> Parent;

	/** Whether this sheet overrides its parent's action list. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions", meta = (InlineEditConditionToggle))
	bool bOverrideActions = true;

	/** Actions available to the agent, with duplicate entries disallowed in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions", meta = (NoElementDuplicate, EditCondition = "bOverrideActions"))
	TArray<TObjectPtr<UActionDefinition>> Actions;

	/** Whether this sheet overrides its parent's starting goal tags. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goals", meta = (InlineEditConditionToggle))
	bool bOverrideStartingGoalTags = true;

	/** Goal tags assigned to the agent's decision component when the AI controller possesses it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goals", meta = (EditCondition = "bOverrideStartingGoalTags"))
	FGameplayTagContainer StartingGoalTags;

	/** Whether this sheet overrides its parent's visual appearance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (InlineEditConditionToggle))
	bool bOverrideAppearance = true;

	/** Visual presentation used by the agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (EditCondition = "bOverrideAppearance"))
	FAppearance Appearance;

	/** Whether this sheet overrides its parent's faction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent", meta = (InlineEditConditionToggle))
	bool bOverrideFaction = true;

	/** Faction this agent belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent", meta = (EditCondition = "bOverrideFaction"))
	EFaction Faction = EFaction::Red;

	/** Whether this sheet overrides its parent's perception settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (InlineEditConditionToggle))
	bool bOverridePerception = true;

	/** Sensory ranges and field of view used by the agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (EditCondition = "bOverridePerception"))
	FPerception Perception;

	/** Whether this sheet overrides its parent's engagement properties. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engage", meta = (InlineEditConditionToggle))
	bool bOverrideEngageProperties = true;

	/** Combat engagement timing and movement preferences. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engage", meta = (EditCondition = "bOverrideEngageProperties"))
	FEngageProperties EngageProperties;

	/** Whether this sheet overrides its parent's cover properties. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover", meta = (InlineEditConditionToggle))
	bool bOverrideCoverProperties = true;

	/** Defensive cover timing preferences. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover", meta = (EditCondition = "bOverrideCoverProperties"))
	FCoverProperties CoverProperties;

	/** Whether this sheet overrides its parent's vitality properties. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vitality", meta = (InlineEditConditionToggle))
	bool bOverrideVitalityProperties = true;

	/** Health values used to initialize this agent's GAS attributes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vitality", meta = (EditCondition = "bOverrideVitalityProperties"))
	FVitalityProperties VitalityProperties;

	/** Whether this sheet overrides its parent's weapon property map. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons", meta = (InlineEditConditionToggle))
	bool bOverrideWeaponProperties = true;

	/** Properties associated with each weapon customization sheet this agent can use. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons", meta = (EditCondition = "bOverrideWeaponProperties"))
	TMap<UWeaponCustomization*, FWeaponProperties> WeaponProperties;

	/** Whether this sheet overrides its parent's tactical-position evaluator map. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical Positioning", meta = (InlineEditConditionToggle))
	bool bOverrideTacticalPositionEvaluators = true;

	/** Firing position evaluators used by each tactical movement mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical Positioning", meta = (EditCondition = "bOverrideTacticalPositionEvaluators"))
	TMap<ETacticalMovementMode, FTacticalMovementModeEvaluatorSet> TacticalPositionEvaluators;

	/** Gets the action list after resolving parent-sheet inheritance. */
	const TArray<TObjectPtr<UActionDefinition>>& GetResolvedActions() const;

	/** Gets starting goal tags after resolving parent-sheet inheritance. */
	const FGameplayTagContainer& GetResolvedStartingGoalTags() const;

	/** Gets appearance settings after resolving parent-sheet inheritance. */
	const FAppearance& GetResolvedAppearance() const;

	/** Gets faction after resolving parent-sheet inheritance. */
	EFaction GetResolvedFaction() const;

	/** Gets perception settings after resolving parent-sheet inheritance. */
	const FPerception& GetResolvedPerception() const;

	/** Gets engagement properties after resolving parent-sheet inheritance. */
	const FEngageProperties& GetResolvedEngageProperties() const;

	/** Gets cover properties after resolving parent-sheet inheritance. */
	const FCoverProperties& GetResolvedCoverProperties() const;

	/** Gets vitality properties after resolving parent-sheet inheritance. */
	const FVitalityProperties& GetResolvedVitalityProperties() const;

	/** Gets weapon properties after resolving parent-sheet inheritance. */
	const TMap<UWeaponCustomization*, FWeaponProperties>& GetResolvedWeaponProperties() const;

	/** Finds weapon properties after resolving parent-sheet inheritance. */
	const FWeaponProperties* FindResolvedWeaponProperties(const UWeaponCustomization* WeaponCustomization) const;

	/** Gets tactical-position evaluators after resolving parent-sheet inheritance. */
	const TMap<ETacticalMovementMode, FTacticalMovementModeEvaluatorSet>& GetResolvedTacticalPositionEvaluators() const;

private:
	const TArray<TObjectPtr<UActionDefinition>>& GetResolvedActions(TSet<const UAgentCustomization*>& Visited) const;
	const FGameplayTagContainer& GetResolvedStartingGoalTags(TSet<const UAgentCustomization*>& Visited) const;
	const FAppearance& GetResolvedAppearance(TSet<const UAgentCustomization*>& Visited) const;
	EFaction GetResolvedFaction(TSet<const UAgentCustomization*>& Visited) const;
	const FPerception& GetResolvedPerception(TSet<const UAgentCustomization*>& Visited) const;
	const FEngageProperties& GetResolvedEngageProperties(TSet<const UAgentCustomization*>& Visited) const;
	const FCoverProperties& GetResolvedCoverProperties(TSet<const UAgentCustomization*>& Visited) const;
	const FVitalityProperties& GetResolvedVitalityProperties(TSet<const UAgentCustomization*>& Visited) const;
	const TMap<UWeaponCustomization*, FWeaponProperties>& GetResolvedWeaponProperties(TSet<const UAgentCustomization*>& Visited) const;
	const TMap<ETacticalMovementMode, FTacticalMovementModeEvaluatorSet>& GetResolvedTacticalPositionEvaluators(TSet<const UAgentCustomization*>& Visited) const;

};
