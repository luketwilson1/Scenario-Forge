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
#include "GrenadeTypes.h"
#include "GameplayTagContainer.h"
#include "AgentCustomization.generated.h"

class UActionDefinition;
class UFiringPositionEval;
class UPawnCustomization;
class UWeaponCustomization;
class UWorldStateQuery;

/**
 * @brief Throw styles available when selecting a grenade trajectory.
 */
UENUM(BlueprintType)
enum class EGrenadeTrajectoryType : uint8
{
	Toss UMETA(DisplayName = "toss"),
	Lob UMETA(DisplayName = "lob"),
	Bounce UMETA(DisplayName = "bounce")
};

/**
 * @brief Defines grenade throwing preferences for an agent.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FGrenadeProperties
{
	GENERATED_BODY()

public:

	/** Throw style used by the grenade action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (DisplayName = "Trajectory Type"))
	EGrenadeTrajectoryType TrajectoryType = EGrenadeTrajectoryType::Toss;

	/** Preferred throw speed, in world units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (Units = "CentimetersPerSecond", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Ideal Throw Speed"))
	float GrenadeIdealVelocity = 0.0f;

	/** Maximum allowed throw speed, in world units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (Units = "CentimetersPerSecond", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Maximum Throw Speed"))
	float GrenadeVelocity = 0.0f;

	/** Minimum throw range, in world units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Minimum Throw Range"))
	float GrenadeMinimumRange = 0.0f;

	/** Maximum throw range, in world units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Maximum Throw Range"))
	float GrenadeMaximumRange = 0.0f;

	/** Percentage chance the agent chooses to throw a grenade when grenade conditions are valid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (Units = "Percent", ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", DisplayName = "Throw Chance"))
	float GrenadeChance = 0.0f;

	/** Delay, in seconds, between grenade throw attempts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Throw Delay"))
	float GrenadeThrowDelay = 0.0f;

	/** Minimum number of enemies required inside EnemyRadius before the agent considers throwing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (ClampMin = "0", UIMin = "0", DisplayName = "Minimum Enemy Count"))
	int32 MinimumEnemyCount = 0;

	/** Radius around a grenade target point used to count clustered enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Enemy Radius"))
	float EnemyRadius = 0.0f;

	/** Radius around the grenade target point that must be clear of friendly agents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Collateral Damage Radius"))
	float CollateralDamageRadius = 0.0f;

	/** Seconds between grenade condition checks while the agent is engaged. Zero uses the controller fallback interval. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade Properties", meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Evaluation Interval"))
	float GrenadeEvalInterval = 0.0f;
};

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
 * @brief Defines how this agent transitions between aim-offset aiming and body turning.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FAimingProperties
{
	GENERATED_BODY()

public:

	/** Maximum yaw, in degrees, the aim offset should handle before body turning is preferred. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming Properties", meta = (Units = "Degrees", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Aim Yaw Limit"))
	float AimYawLimit = 0.0f;

	/** Maximum pitch, in degrees, the aim offset should handle before alternate behavior is preferred. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming Properties", meta = (Units = "Degrees", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Aim Pitch Limit"))
	float AimPitchLimit = 0.0f;

	/** Body yaw adjustment, in degrees, to apply when the target exceeds the aim yaw limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming Properties", meta = (Units = "Degrees", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Body Turn Step Degrees"))
	float BodyTurnStepDegrees = 0.0f;

	/** Body turning speed, in degrees per second, used when smoothly rotating toward the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming Properties", meta = (Units = "DegreesPerSecond", ClampMin = "0.0", UIMin = "0.0", DisplayName = "Body Turn Speed"))
	float BodyTurnSpeed = 0.0f;
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
 * @brief Defines how this agent reacts to nearby danger sources.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FDodgeProperties
{
	GENERATED_BODY()

public:

	/** Distance, in centimeters, this agent tries to move when dodging danger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge Properties", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float Distance = 0.0f;

	/** Movement speed, in centimeters per second, used to estimate temporary dodge state duration when animation has no notify. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge Properties", meta = (Units = "CentimetersPerSecond", ClampMin = "0.0", UIMin = "0.0"))
	float Speed = 0.0f;

	/** Delay, in seconds, before this agent reacts to a detected danger source. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge Properties", meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0"))
	float ReactionDelay = 0.0f;

	/** Delay, in seconds, before this agent may choose another danger dodge action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge Properties", meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0"))
	float Cooldown = 0.0f;
};

/**
 * @brief Selects planner goals when an agent's current state matches designer-authored conditions.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FGoalSelectionRule
{
	GENERATED_BODY()

public:

	/** Optional label used to identify this rule in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal Rule")
	FName RuleName = NAME_None;

	/** States that must all be present before this rule can select goals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal Rule")
	FGameplayTagContainer RequiredStates;

	/** States that prevent this rule from selecting goals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal Rule")
	FGameplayTagContainer BlockedStates;

	/** Goals the planner should satisfy while this rule is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal Rule")
	FGameplayTagContainer GoalStates;

	/** Higher scoring rules win when multiple rules match the current state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal Rule", meta = (DisplayName = "Score"))
	int32 Priority = 0;
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

	/** Draws debug visualization for this evaluator while scoring candidate positions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluator|Debug")
	bool bDebugDraw = false;

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

	/** Initializes default sheet query mappings. */
	UAgentCustomization();

	/** Parent sheet used as a fallback for sections this sheet does not override. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inheritance", meta = (DisplayPriority = "0"))
	TObjectPtr<UAgentCustomization> Parent;

	/** Whether this sheet overrides its parent's pawn customization sheet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pawn", meta = (InlineEditConditionToggle))
	bool bOverridePawnCustomization = false;

	/** Pawn presentation sheet used by this agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pawn", meta = (EditCondition = "bOverridePawnCustomization"))
	TObjectPtr<UPawnCustomization> PawnCustomization;

	/** Whether this sheet overrides its parent's grenade properties. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade Properties", meta = (InlineEditConditionToggle))
	bool bOverrideGrenadeProperties = false;

	/** Grenade gameplay defaults used by the agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade Properties", meta = (EditCondition = "bOverrideGrenadeProperties"))
	TMap<EGrenadeType, FGrenadeProperties> GrenadeProperties;

	/** Whether this sheet overrides its parent's default primary weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Starting Equipment", meta = (InlineEditConditionToggle))
	bool bOverrideDefaultPrimaryWeapon = false;

	/** Primary weapon sheet assigned to the agent when play begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Starting Equipment", meta = (EditCondition = "bOverrideDefaultPrimaryWeapon"))
	TObjectPtr<UWeaponCustomization> DefaultPrimaryWeapon;

	/** Whether this sheet overrides its parent's default secondary weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Starting Equipment", meta = (InlineEditConditionToggle))
	bool bOverrideDefaultSecondaryWeapon = false;

	/** Secondary weapon sheet assigned to the agent when play begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Starting Equipment", meta = (EditCondition = "bOverrideDefaultSecondaryWeapon"))
	TObjectPtr<UWeaponCustomization> DefaultSecondaryWeapon;

	/** Whether this sheet overrides its parent's starting grenade inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Starting Equipment", meta = (InlineEditConditionToggle))
	bool bOverrideStartingGrenades = false;

	/** Grenade counts assigned to the agent when play begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Starting Equipment", meta = (EditCondition = "bOverrideStartingGrenades"))
	TArray<FStartingGrenade> StartingGrenades;

	/** Whether this sheet overrides its parent's action list. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions", meta = (InlineEditConditionToggle))
	bool bOverrideActions = false;

	/** Actions available to the agent, with duplicate entries disallowed in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions", meta = (NoElementDuplicate, EditCondition = "bOverrideActions"))
	TArray<TObjectPtr<UActionDefinition>> Actions;

	/** Whether this sheet overrides its parent's starting goal tags. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goals", meta = (InlineEditConditionToggle))
	bool bOverrideStartingGoalTags = false;

	/** Goal tags assigned to the agent's decision component when the AI controller possesses it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goals", meta = (EditCondition = "bOverrideStartingGoalTags"))
	FGameplayTagContainer StartingGoalTags;

	/** Whether this sheet overrides its parent's state-driven goal selection rules. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goals", meta = (InlineEditConditionToggle))
	bool bOverrideGoalSelectionRules = false;

	/** Rules that select planner goals from the agent's current state. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goals", meta = (EditCondition = "bOverrideGoalSelectionRules"))
	TArray<FGoalSelectionRule> GoalSelectionRules;

	/** Whether this sheet overrides its parent's state query map. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State Queries", meta = (InlineEditConditionToggle))
	bool bOverrideStateQueries = false;

	/** Runtime queries used to resolve whether specific GOAP state tags are currently true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State Queries", meta = (EditCondition = "bOverrideStateQueries"))
	TMap<FGameplayTag, TSubclassOf<UWorldStateQuery>> StateQueries;

	/** Whether this sheet overrides its parent's faction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent", meta = (InlineEditConditionToggle))
	bool bOverrideFaction = false;

	/** Faction this agent belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent", meta = (EditCondition = "bOverrideFaction"))
	EFaction Faction = EFaction::Red;

	/** Whether this sheet overrides its parent's perception settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (InlineEditConditionToggle))
	bool bOverridePerception = false;

	/** Sensory ranges and field of view used by the agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (EditCondition = "bOverridePerception"))
	FPerception Perception;

	/** Whether this sheet overrides its parent's aiming settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming Properties", meta = (InlineEditConditionToggle))
	bool bOverrideAimingProperties = false;

	/** Aim-offset and body-turning preferences used by this agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming Properties", meta = (EditCondition = "bOverrideAimingProperties"))
	FAimingProperties AimingProperties;

	/** Whether this sheet overrides its parent's engagement properties. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engage", meta = (InlineEditConditionToggle))
	bool bOverrideEngageProperties = false;

	/** Combat engagement timing and movement preferences. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engage", meta = (EditCondition = "bOverrideEngageProperties"))
	FEngageProperties EngageProperties;

	/** Whether this sheet overrides its parent's cover properties. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover", meta = (InlineEditConditionToggle))
	bool bOverrideCoverProperties = false;

	/** Defensive cover timing preferences. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover", meta = (EditCondition = "bOverrideCoverProperties"))
	FCoverProperties CoverProperties;

	/** Whether this sheet overrides its parent's dodge properties. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge Properties", meta = (InlineEditConditionToggle))
	bool bOverrideDodgeProperties = false;

	/** Danger reaction movement preferences. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge Properties", meta = (EditCondition = "bOverrideDodgeProperties"))
	FDodgeProperties DodgeProperties;

	/** Whether this sheet overrides its parent's vitality properties. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vitality", meta = (InlineEditConditionToggle))
	bool bOverrideVitalityProperties = false;

	/** Health values used to initialize this agent's GAS attributes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vitality", meta = (EditCondition = "bOverrideVitalityProperties"))
	FVitalityProperties VitalityProperties;

	/** Whether this sheet overrides its parent's weapon property map. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons", meta = (InlineEditConditionToggle))
	bool bOverrideWeaponProperties = false;

	/** Properties associated with each weapon customization sheet this agent can use. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons", meta = (EditCondition = "bOverrideWeaponProperties"))
	TMap<UWeaponCustomization*, FWeaponProperties> WeaponProperties;

	/** Whether this sheet overrides its parent's tactical-position evaluator map. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical Positioning", meta = (InlineEditConditionToggle))
	bool bOverrideTacticalPositionEvaluators = false;

	/** Firing position evaluators used by each tactical movement mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical Positioning", meta = (EditCondition = "bOverrideTacticalPositionEvaluators"))
	TMap<ETacticalMovementMode, FTacticalMovementModeEvaluatorSet> TacticalPositionEvaluators;

	/** Gets the action list after resolving parent-sheet inheritance. */
	const TArray<TObjectPtr<UActionDefinition>>& GetResolvedActions() const;

	/** Gets starting goal tags after resolving parent-sheet inheritance. */
	const FGameplayTagContainer& GetResolvedStartingGoalTags() const;

	/** Gets state-driven goal selection rules after resolving parent-sheet inheritance. */
	const TArray<FGoalSelectionRule>& GetResolvedGoalSelectionRules() const;

	/** Gets state queries after resolving parent-sheet inheritance. */
	const TMap<FGameplayTag, TSubclassOf<UWorldStateQuery>>& GetResolvedStateQueries() const;

	/** Gets grenade properties by grenade type after resolving parent-sheet inheritance. */
	const TMap<EGrenadeType, FGrenadeProperties>& GetResolvedGrenadeProperties() const;

	/** Finds grenade properties by grenade type after resolving parent-sheet inheritance. */
	const FGrenadeProperties* FindResolvedGrenadeProperties(EGrenadeType GrenadeType) const;

	/** Gets starting grenade inventory after resolving parent-sheet inheritance. */
	const TArray<FStartingGrenade>& GetResolvedStartingGrenades() const;

	/** Gets the default primary weapon after resolving parent-sheet inheritance. */
	UWeaponCustomization* GetResolvedDefaultPrimaryWeapon() const;

	/** Gets the default secondary weapon after resolving parent-sheet inheritance. */
	UWeaponCustomization* GetResolvedDefaultSecondaryWeapon() const;

	/** Gets pawn customization after resolving parent-sheet inheritance. */
	const UPawnCustomization* GetResolvedPawnCustomization() const;

	/** Gets appearance settings after resolving parent-sheet inheritance. */
	const FAppearance& GetResolvedAppearance() const;

	/** Gets faction after resolving parent-sheet inheritance. */
	EFaction GetResolvedFaction() const;

	/** Gets perception settings after resolving parent-sheet inheritance. */
	const FPerception& GetResolvedPerception() const;

	/** Gets aiming settings after resolving parent-sheet inheritance. */
	const FAimingProperties& GetResolvedAimingProperties() const;

	/** Gets engagement properties after resolving parent-sheet inheritance. */
	const FEngageProperties& GetResolvedEngageProperties() const;

	/** Gets cover properties after resolving parent-sheet inheritance. */
	const FCoverProperties& GetResolvedCoverProperties() const;

	/** Gets dodge properties after resolving parent-sheet inheritance. */
	const FDodgeProperties& GetResolvedDodgeProperties() const;

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
	const TArray<FGoalSelectionRule>& GetResolvedGoalSelectionRules(TSet<const UAgentCustomization*>& Visited) const;
	const TMap<FGameplayTag, TSubclassOf<UWorldStateQuery>>& GetResolvedStateQueries(TSet<const UAgentCustomization*>& Visited) const;
	const TMap<EGrenadeType, FGrenadeProperties>& GetResolvedGrenadeProperties(TSet<const UAgentCustomization*>& Visited) const;
	const TArray<FStartingGrenade>& GetResolvedStartingGrenades(TSet<const UAgentCustomization*>& Visited) const;
	UWeaponCustomization* GetResolvedDefaultPrimaryWeapon(TSet<const UAgentCustomization*>& Visited) const;
	UWeaponCustomization* GetResolvedDefaultSecondaryWeapon(TSet<const UAgentCustomization*>& Visited) const;
	const UPawnCustomization* GetResolvedPawnCustomization(TSet<const UAgentCustomization*>& Visited) const;
	const FAppearance& GetResolvedAppearance(TSet<const UAgentCustomization*>& Visited) const;
	EFaction GetResolvedFaction(TSet<const UAgentCustomization*>& Visited) const;
	const FPerception& GetResolvedPerception(TSet<const UAgentCustomization*>& Visited) const;
	const FAimingProperties& GetResolvedAimingProperties(TSet<const UAgentCustomization*>& Visited) const;
	const FEngageProperties& GetResolvedEngageProperties(TSet<const UAgentCustomization*>& Visited) const;
	const FCoverProperties& GetResolvedCoverProperties(TSet<const UAgentCustomization*>& Visited) const;
	const FDodgeProperties& GetResolvedDodgeProperties(TSet<const UAgentCustomization*>& Visited) const;
	const FVitalityProperties& GetResolvedVitalityProperties(TSet<const UAgentCustomization*>& Visited) const;
	const TMap<UWeaponCustomization*, FWeaponProperties>& GetResolvedWeaponProperties(TSet<const UAgentCustomization*>& Visited) const;
	const TMap<ETacticalMovementMode, FTacticalMovementModeEvaluatorSet>& GetResolvedTacticalPositionEvaluators(TSet<const UAgentCustomization*>& Visited) const;

};
