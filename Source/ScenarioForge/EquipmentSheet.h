// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file EquipmentSheet.h
 * @brief Declares the data asset used to customize usable equipment.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GrenadeTypes.h"
#include "EquipmentSheet.generated.h"

class AActor;
class UProjectileSheet;
class UStaticMesh;

/**
 * @brief Broad equipment family used by inventory and AI queries.
 */
UENUM(BlueprintType)
enum class EEquipmentCategory : uint8
{
	Generic UMETA(DisplayName = "Generic"),
	Grenade UMETA(DisplayName = "Grenade")
};

/**
 * @brief Defines the runtime behavior used when equipment is activated.
 */
UENUM(BlueprintType)
enum class EEquipmentUseBehavior : uint8
{
	None UMETA(DisplayName = "None"),
	SpawnProjectile UMETA(DisplayName = "Spawn Projectile"),
	SpawnActor UMETA(DisplayName = "Spawn Actor")
};

/**
 * @brief Stores configurable data for an equipment item.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API UEquipmentSheet : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Stable designer-authored equipment identifier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FGameplayTag EquipmentId;

	/** User-facing equipment name for editor and UI display. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FText DisplayName;

	/** Broad equipment family. Grenades can be selected by grenade type for AI throw logic. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	EEquipmentCategory Category = EEquipmentCategory::Generic;

	/** Grenade type represented by this equipment sheet when Category is Grenade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Grenade", meta = (EditCondition = "Category == EEquipmentCategory::Grenade"))
	EGrenadeType GrenadeType = EGrenadeType::None;

	/** Maximum count an inventory should carry for this equipment. Zero means uncapped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxCount = 0;

	/** Mesh used for held or pickup presentation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Visuals")
	TObjectPtr<UStaticMesh> StaticMesh;

	/** Runtime behavior this equipment performs when used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Use")
	EEquipmentUseBehavior UseBehavior = EEquipmentUseBehavior::None;

	/** Projectile sheet spawned when UseBehavior is Spawn Projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Use", meta = (EditCondition = "UseBehavior == EEquipmentUseBehavior::SpawnProjectile"))
	TObjectPtr<UProjectileSheet> ProjectileToSpawn;

	/** Actor class spawned when UseBehavior is Spawn Actor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Use", meta = (EditCondition = "UseBehavior == EEquipmentUseBehavior::SpawnActor"))
	TSubclassOf<AActor> ActorToSpawn;
};
