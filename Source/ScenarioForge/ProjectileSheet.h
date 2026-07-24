// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ProjectileSheet.h
 * @brief Declares the data asset used to customize a projectile.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectileSheet.generated.h"

class UStaticMesh;
class UParticleSystem;
class UDamageEffectSheet;

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
 * @brief Defines which system controls projectile movement after launch.
 */
UENUM(BlueprintType)
enum class EProjectileMovementMode : uint8
{
	ProjectileMovementComponent UMETA(DisplayName = "Projectile Movement Component"),
	SimulatedPhysics UMETA(DisplayName = "Simulated Physics")
};

/**
 * @brief Defines which primitive provides projectile collision.
 */
UENUM(BlueprintType)
enum class EProjectileCollisionSource : uint8
{
	SimpleSphere UMETA(DisplayName = "Simple Sphere"),
	StaticMeshCollision UMETA(DisplayName = "Static Mesh Collision")
};

/**
 * @brief Stores configurable data for a projectile.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API UProjectileSheet : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Static mesh displayed by the projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UStaticMesh> StaticMesh;

	/** Default visual effect spawned when the projectile impacts something. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UParticleSystem> ImpactVFX;

	/** Visual effect spawned when the projectile detonates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visuals")
	TObjectPtr<UParticleSystem> DetonationVFX;

	/** Primitive used for projectile collision and physics. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Collision")
	EProjectileCollisionSource CollisionSource = EProjectileCollisionSource::SimpleSphere;

	/** Radius, in centimeters, used by the projectile collision sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Collision", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0", EditCondition = "CollisionSource == EProjectileCollisionSource::SimpleSphere"))
	float CollisionRadius = 2.0f;

	/** System used to move this projectile after it is launched. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Movement")
	EProjectileMovementMode MovementMode = EProjectileMovementMode::ProjectileMovementComponent;

	/** Initial projectile speed, in centimeters per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Movement", meta = (Units = "CentimetersPerSecond", ClampMin = "0.0", UIMin = "0.0", EditCondition = "MovementMode == EProjectileMovementMode::ProjectileMovementComponent"))
	float InitialSpeed = 10000.0f;

	/** Maximum projectile speed, in centimeters per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Movement", meta = (Units = "CentimetersPerSecond", ClampMin = "0.0", UIMin = "0.0", EditCondition = "MovementMode == EProjectileMovementMode::ProjectileMovementComponent"))
	float MaxSpeed = 10000.0f;

	/** Gravity multiplier applied to the projectile movement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "MovementMode == EProjectileMovementMode::ProjectileMovementComponent"))
	float GravityScale = 0.0f;

	/** Linear damping applied when this projectile uses simulated physics. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Physics", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "MovementMode == EProjectileMovementMode::SimulatedPhysics"))
	float PhysicsLinearDamping = 0.1f;

	/** Angular damping applied when this projectile uses simulated physics. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Physics", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "MovementMode == EProjectileMovementMode::SimulatedPhysics"))
	float PhysicsAngularDamping = 0.1f;

	/** Initial angular velocity, in degrees per second, applied when using simulated physics. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Physics", meta = (Units = "DegreesPerSecond", EditCondition = "MovementMode == EProjectileMovementMode::SimulatedPhysics"))
	FVector PhysicsAngularVelocity = FVector(0.0f, 720.0f, 360.0f);

	/** Maximum projectile lifetime, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0"))
	float MaxLifetime = 3.0f;

	/** Damage effect applied when the projectile hits a valid target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UDamageEffectSheet> DamageEffect;

	/** Creates an overlap volume around this projectile using the damage effect radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Danger")
	bool bCreateGrenadeDangerVolume = false;

	/** Draws the grenade danger volume while this projectile exists. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Danger", meta = (EditCondition = "bCreateGrenadeDangerVolume"))
	bool bDrawGrenadeDangerDebug = false;

	/** Behavior used after this projectile hits blocking geometry or an actor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Impact")
	EProjectileImpactBehavior ImpactBehavior = EProjectileImpactBehavior::DestroyOnImpact;

	/** Condition that causes this projectile to detonate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Detonation")
	EProjectileDetonationTrigger DetonationTrigger = EProjectileDetonationTrigger::None;

	/** Delay, in seconds, before timed detonation fires. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Detonation", meta = (Units = "Seconds", ClampMin = "0.0", UIMin = "0.0", EditCondition = "DetonationTrigger == EProjectileDetonationTrigger::Timed"))
	float DetonationTimer = 0.0f;

	/** Draws the damage effect's outer detonation radius briefly when this projectile detonates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Detonation", meta = (EditCondition = "DetonationTrigger != EProjectileDetonationTrigger::None"))
	bool bDrawDetonationRadiusDebug = false;
};
