// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Area.h
 * @brief Declares authored tactical areas that group related firing positions.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Area.generated.h"

class AFiringPosition;
class UStaticMeshComponent;

/**
 * @brief Designer-authored tactical area containing firing positions for a role or front.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API AArea : public AActor
{
	GENERATED_BODY()

public:
	/** Initializes this tactical area actor. */
	AArea();

	/** Applies the configured area material and parameter values to the cube mesh. */
	void ApplyAreaMaterial();

	/** Generates firing positions inside this area using the area generation settings. */
	UFUNCTION(CallInEditor, Category = "AI|Firing Position Area", meta = (DisplayName = "Generate Firing Positions"))
	void GenerateFiringPositions();

	/** Cube mesh representing this authored tactical area. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Firing Position Area")
	TObjectPtr<UStaticMeshComponent> AreaMeshComponent;

	/** Randomized display color passed to the area material. */
	UPROPERTY()
	FLinearColor AreaBaseColor = FLinearColor::Red;

	/** Display opacity passed to the area material. */
	UPROPERTY()
	float Opacity = 0.10f;

	/** Designer-facing tactical label, such as Sniper, Frontline, or Flank Left. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Firing Position Area")
	FName AreaName = NAME_None;

	/** Firing positions grouped under this tactical area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Firing Position Area", meta = (NoElementDuplicate))
	TArray<TObjectPtr<AFiringPosition>> FiringPositions;

	/** Number of firing positions to try to maintain when generating positions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Firing Position Area|Generation", meta = (ClampMin = "0", UIMin = "0"))
	int32 GeneratedFiringPositionCount = 24;

	/** Maximum random candidate attempts for generation before giving up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Firing Position Area|Generation", meta = (ClampMin = "1", UIMin = "1"))
	int32 FiringPositionGenerationMaxAttempts = 500;

	/** Minimum horizontal spacing between generated firing positions, in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Firing Position Area|Generation", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float GeneratedFiringPositionMinSpacing = 150.0f;

	/** Half-size of the collision clearance box used to reject blocked generated positions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Firing Position Area|Generation", meta = (Units = "Centimeters", ClampMin = "1.0", UIMin = "1.0"))
	FVector GeneratedFiringPositionClearanceHalfExtent = FVector(25.0f, 25.0f, 10.0f);

	/** Distance to lift generated firing positions off the hit surface to avoid z-fighting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Firing Position Area|Generation", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float GeneratedFiringPositionSurfaceOffset = 1.0f;

protected:
	/** Resets saved runtime visibility so debug visuals start hidden in game. */
	virtual void PostLoad() override;

	/** Reapplies material parameters when editable area data changes. */
	virtual void OnConstruction(const FTransform& Transform) override;
};
