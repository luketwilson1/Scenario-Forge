// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FiringPosition.h
 * @brief Declares authored tactical firing positions used by AI position evaluators.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FiringPosition.generated.h"

class AArea;
class UStaticMeshComponent;

/**
 * @brief Cover and placement properties describing a firing position.
 */
UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EFiringPositionFlag : uint8
{
	None = 0 UMETA(Hidden),
	Open = 1 << 0 UMETA(DisplayName = "Open"),
	Partial = 1 << 1 UMETA(DisplayName = "Partial"),
	Closed = 1 << 2 UMETA(DisplayName = "Closed"),
	WallLean = 1 << 3 UMETA(DisplayName = "Wall Lean"),
	Perch = 1 << 4 UMETA(DisplayName = "Perch"),
	GroundPoint = 1 << 5 UMETA(DisplayName = "Ground Point"),
	AutomaticallyGenerated = 1 << 6 UMETA(DisplayName = "Automatically Generated")
};

ENUM_CLASS_FLAGS(EFiringPositionFlag);

/**
 * @brief Posture constraints supported by a firing position.
 *
 * These are stored in AFiringPosition::PostureFlags as a bitmask and are kept
 * separate from behavior flags so AI can filter cover/posture preferences.
 */
UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EFiringPositionPostureFlag : uint8
{
	None = 0 UMETA(Hidden),
	CornerLeft = 1 << 0 UMETA(DisplayName = "Corner Left"),
	CornerRight = 1 << 1 UMETA(DisplayName = "Corner Right"),
	Bunker = 1 << 2 UMETA(DisplayName = "Bunker"),
	BunkerHigh = 1 << 3 UMETA(DisplayName = "Bunker High"),
	BunkerLow = 1 << 4 UMETA(DisplayName = "Bunker Low")
};

ENUM_CLASS_FLAGS(EFiringPositionPostureFlag);

/**
 * @brief Authored tactical point that can be scored by AI firing-position evaluators.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API AFiringPosition : public AActor
{
	GENERATED_BODY()

public:
	/** Initializes this firing position actor. */
	AFiringPosition();

	/** Applies the configured firing-position material and parameter values. */
	void ApplyFiringPositionMaterial();

	/** Plane mesh representing this authored firing position. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Firing Position")
	TObjectPtr<UStaticMeshComponent> FiringPositionMeshComponent;

	/** Display color passed to the firing-position material. */
	UPROPERTY()
	FLinearColor FiringPositionBaseColor = FLinearColor::Red;

	/** Display opacity passed to the firing-position material. */
	UPROPERTY()
	float Opacity = 1.0f;

	/** Cover and placement properties for this firing position. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Firing Position", meta = (Bitmask, BitmaskEnum = "/Script/ScenarioForge.EFiringPositionFlag"))
	int32 PositionFlags = static_cast<int32>(EFiringPositionFlag::None);

	/** Posture constraints supported by this firing position. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Firing Position", meta = (Bitmask, BitmaskEnum = "/Script/ScenarioForge.EFiringPositionPostureFlag"))
	int32 PostureFlags = 0;

protected:
	/** Resets saved runtime visibility so debug visuals start hidden in game. */
	virtual void PostLoad() override;

	/** Reapplies material parameters when editable firing position data changes. */
	virtual void OnConstruction(const FTransform& Transform) override;
};
