// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ProjectileCustomization.h
 * @brief Declares the data asset used to customize a projectile.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectileCustomization.generated.h"

class UStaticMesh;
class UParticleSystem;
class UDamageEffectCustomization;

/**
 * @brief Defines what a projectile should do after a blocking impact.
 */
UENUM(BlueprintType)
enum class EProjectileImpactBehavior : uint8
{
	DestroyOnImpact UMETA(DisplayName = "Destroy On Impact"),
	Bounce UMETA(DisplayName = "Bounce"),
	Detonate UMETA(DisplayName = "Detonate"),
	Stick UMETA(DisplayName = "Stick"),
	Penetrate UMETA(DisplayName = "Penetrate")
};

/**
 * @brief Defines what condition triggers projectile detonation.
 */
UENUM(BlueprintType)
enum class EProjectileDetonationTrigger : uint8
{
	None UMETA(DisplayName = "None"),
	OnImpact UMETA(DisplayName = "On Impact"),
	Timed UMETA(DisplayName = "Timed")
};

/**
 * @brief Stores configurable data for a projectile.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API UProjectileCustomization : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Static mesh displayed by the projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UStaticMesh> StaticMesh;

	/** Default visual effect spawned when the projectile impacts something. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UParticleSystem> ImpactVFX;

	/** Radius, in centimeters, used by the projectile collision sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Collision", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float CollisionRadius = 2.0f;

	/** Initial projectile speed, in centimeters per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Movement", meta = (Units = "CentimetersPerSecond", ClampMin = "0.0", UIMin = "0.0"))
	float InitialSpeed = 10000.0f;

	/** Maximum projectile speed, in centimeters per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Movement", meta = (Units = "CentimetersPerSecond", ClampMin = "0.0", UIMin = "0.0"))
	float MaxSpeed = 10000.0f;

	/** Gravity multiplier applied to the projectile movement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GravityScale = 0.0f;

	/** Maximum projectile lifetime, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0"))
	float MaxLifetime = 3.0f;

	/** Damage effect applied when the projectile hits a valid target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UDamageEffectCustomization> DamageEffect;

	/** Behavior used after this projectile hits blocking geometry or an actor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Impact")
	EProjectileImpactBehavior ImpactBehavior = EProjectileImpactBehavior::DestroyOnImpact;

	/** Condition that causes this projectile to detonate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Detonation")
	EProjectileDetonationTrigger DetonationTrigger = EProjectileDetonationTrigger::None;

	/** Delay, in seconds, before timed detonation fires. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Detonation", meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0", EditCondition = "DetonationTrigger == EProjectileDetonationTrigger::Timed"))
	float DetonationTimer = 0.0f;
};
