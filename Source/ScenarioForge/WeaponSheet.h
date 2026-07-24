// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file WeaponSheet.h
 * @brief Declares the data asset used to customize a weapon.
 */

#pragma once

#include "CoreMinimal.h"
#include "SheetTypes.h"
#include "Engine/DataAsset.h"
#include "WeaponSheet.generated.h"

class UParticleSystem;
class UProjectileSheet;

/**
 * @brief Describes the firing or cycling mechanism used by a weapon.
 */
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	/** Fires once for each trigger pull and cycles automatically. */
	SemiAutomatic UMETA(DisplayName = "Semi Auto"),

	/** Requires the bolt to be manually cycled between shots. */
	BoltAction UMETA(DisplayName = "Bolt Action"),

	/** Continues firing while the trigger is held. */
	FullyAutomatic UMETA(DisplayName = "Full Auto"),

	/** Requires the pump to be manually cycled between shots. */
	PumpAction UMETA(DisplayName = "Pump Action")
};

/**
 * @brief Describes how the weapon should be held by an agent.
 */
UENUM(BlueprintType)
enum class EWeaponGripStyle : uint8
{
	/** One-handed weapon held in the right hand. */
	Pistol UMETA(DisplayName = "Pistol"),

	/** Two-handed weapon attached to the right hand, with left-hand IK handled later. */
	Rifle UMETA(DisplayName = "Rifle")
};

/**
 * @brief Stores configurable data for a weapon.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API UWeaponSheet : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Visual presentation used by the weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAppearance Appearance;

	/** Firing or cycling mechanism used by this weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::SemiAutomatic;

	/** How this weapon should be held by an agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EWeaponGripStyle GripStyle = EWeaponGripStyle::Rifle;

	/** Maximum number of rounds the weapon can fire per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0", UIMin = "0", DisplayName = "Rate of Fire (Rounds/Second)"))
	int32 RateOfFire = 0;

	/** Projectile spawned when this weapon fires. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	TObjectPtr<UProjectileSheet> ProjectileSheet;

	/** Socket used as the projectile and muzzle effect spawn point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** Particle effect spawned at the muzzle when the weapon fires. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|VFX")
	TObjectPtr<UParticleSystem> MuzzleFlashVFX;
};
