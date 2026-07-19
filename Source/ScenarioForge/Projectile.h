// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Projectile.h
 * @brief Declares the runtime projectile actor fired by weapons.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileCustomization.h"
#include "Projectile.generated.h"

class UProjectileCustomization;
class UProjectileMovementComponent;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class UDamageEffectCustomization;

/**
 * @brief Runtime projectile actor configured from a projectile customization asset.
 */
UCLASS()
class SCENARIOFORGE_API AProjectile : public AActor
{
	GENERATED_BODY()

public:

	/** Initializes projectile collision, visual mesh, movement, and lifetime defaults. */
	AProjectile();

	/** Draws optional runtime projectile debug visuals. */
	virtual void Tick(float DeltaTime) override;

	/** Clears any danger state granted by this projectile before it leaves the world. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
 * @brief Applies projectile movement, visuals, damage, impact VFX, and detonation VFX settings.
	 *
	 * @param ProjectileCustomization Customization data to apply to this projectile.
	 */
	void ApplyProjectileCustomization(const UProjectileCustomization* ProjectileCustomization);

	/**
	 * @brief Launches the projectile using the movement mode configured by its projectile sheet.
	 *
	 * @param InitialVelocity World-space velocity to apply on launch.
	 */
	void Launch(const FVector& InitialVelocity);

	/** Gets the outer damage radius used to determine whether a safety point is outside this explosion. */
	float GetDetonationOuterRadius() const;

	/** Returns whether this projectile represents an explosive danger source rather than ordinary bullet fire. */
	bool IsGrenadeDangerProjectile() const;

	/** Collision sphere used to detect projectile hits. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	/** Static mesh displayed by the projectile. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	/** Movement component that advances the projectile through the world. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	/** Overlap volume used to detect agents inside grenade danger range. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> GrenadeDangerComponent;

	/** Damage effect applied when this projectile hits a valid target. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UDamageEffectCustomization> DamageEffect;

protected:

	/** Default visual effect spawned when this projectile impacts something. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UParticleSystem> ImpactVFX;

	/** Visual effect spawned when this projectile detonates. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UParticleSystem> DetonationVFX;

	/** Behavior used after this projectile hits blocking geometry or an actor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	EProjectileImpactBehavior ImpactBehavior = EProjectileImpactBehavior::DestroyOnImpact;

	/** Condition that causes this projectile to detonate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	EProjectileDetonationTrigger DetonationTrigger = EProjectileDetonationTrigger::None;

	/** Delay, in seconds, before timed detonation fires. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	float DetonationTimer = 0.0f;

	/** Whether to draw the configured outer damage radius when this projectile detonates. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	bool bDrawDetonationRadiusDebug = false;

	/** System currently configured to move this projectile after launch. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	EProjectileMovementMode MovementMode = EProjectileMovementMode::ProjectileMovementComponent;

	/** Primitive currently configured to provide collision and physics. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	EProjectileCollisionSource CollisionSource = EProjectileCollisionSource::SimpleSphere;

	/** Initial angular velocity used when this projectile launches with simulated physics. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector PhysicsAngularVelocity = FVector::ZeroVector;

	/** Whether this projectile creates a grenade danger overlap volume. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	bool bCreateGrenadeDangerVolume = false;

	/** Whether to draw the grenade danger volume for debugging. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	bool bDrawGrenadeDangerDebug = false;

	/** Timer used for timed projectile detonation. */
	FTimerHandle DetonationTimerHandle;

	/** Agents currently marked as inside this projectile's grenade danger radius. */
	TSet<TWeakObjectPtr<class AAgent>> AgentsInGrenadeDanger;

	/** Spawns detonation VFX, applies radial damage, and destroys this projectile. */
	UFUNCTION()
	void Detonate();

	/** Configures the grenade danger overlap volume from projectile data. */
	void ConfigureGrenadeDangerVolume();

	/** Applies the current collision source choice to the runtime components. */
	void ConfigureCollisionSource();

	/** Returns the primitive currently responsible for collision and physics. */
	UPrimitiveComponent* GetActiveCollisionPrimitive() const;

	/** Applies this projectile's damage effect around the detonation point. */
	void ApplyDetonationDamage();

	/** Adds or removes this projectile's danger tags from an agent. */
	void SetGrenadeDangerState(AAgent* Agent, bool bInDanger);

	/** Handles actors entering the grenade danger volume. */
	UFUNCTION()
	void HandleGrenadeDangerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	/** Handles actors leaving the grenade danger volume. */
	UFUNCTION()
	void HandleGrenadeDangerEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	/**
	 * @brief Handles blocking hit events, applies damage, spawns impact VFX, and destroys the projectile.
	 *
	 * @param HitComponent Component on this projectile that generated the hit.
	 * @param OtherActor Actor hit by the projectile.
	 * @param OtherComponent Component hit on the other actor.
	 * @param NormalImpulse Physics impulse reported by the hit event.
	 * @param Hit Detailed hit result supplied by Unreal.
	 */
	UFUNCTION()
	void HandleHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);
};
