// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file CustomizationTypes.h
 * @brief Declares shared data structures used by customization data assets.
 */

#pragma once

#include "CoreMinimal.h"
#include "CustomizationTypes.generated.h"

class UAnimInstance;
class UMaterialInterface;
class USkeletalMesh;

/**
 * @brief Overrides one material slot on a customized mesh.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FMaterialOverride
{
	GENERATED_BODY()

public:

	/** Material slot index to override on the customized mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	int32 MaterialSlotIndex = 0;

	/** Material assigned to the slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> Material;
};

/**
 * @brief Defines the skeletal mesh and animation blueprint used to present an actor or item.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FAppearance
{
	GENERATED_BODY()

public:

	/** Skeletal mesh displayed by the customized object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	/** Animation Blueprint class used to animate the skeletal mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AnimationBlueprint;

	// TODO TRAFFIC CONE: Later replace this manual list with editor UI that shows the actual material slots for SkeletalMesh.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FMaterialOverride> MaterialOverrides;
};
