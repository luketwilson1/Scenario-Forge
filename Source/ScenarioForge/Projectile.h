// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Projectile.h
 * @brief Declares the runtime projectile actor fired by weapons.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UProjectileCustomization;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;

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

	/**
	 * @brief Applies projectile movement, visuals, damage, and impact VFX settings.
	 *
	 * @param ProjectileCustomization Customization data to apply to this projectile.
	 */
	void ApplyProjectileCustomization(const UProjectileCustomization* ProjectileCustomization);

	/** Collision sphere used to detect projectile hits. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	/** Static mesh displayed by the projectile. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	/** Movement component that advances the projectile through the world. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	/** Damage applied when this projectile hits a valid target. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	float Damage = 0.0f;

protected:

	/** Default visual effect spawned when this projectile impacts something. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UParticleSystem> ImpactVFX;

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
