// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Weapon.h
 * @brief Declares the runtime weapon actor used by agents.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class UAbilitySystemComponent;
class USkeletalMeshComponent;
class UWeaponCustomization;

/**
 * @brief Runtime weapon actor that applies weapon customization and fires projectiles.
 */
UCLASS()
class SCENARIOFORGE_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	/** Initializes the weapon actor and its skeletal mesh root component. */
	AWeapon();

	/**
	 * @brief Applies the configured mesh, animation, materials, projectile, and VFX data.
	 *
	 * @param WeaponCustomization Customization data to apply to this weapon.
	 */
	void ApplyWeaponCustomization(UWeaponCustomization* WeaponCustomization);

	/**
	 * @brief Fires one projectile toward a world-space target location.
	 *
	 * @param TargetLocation World-space point used to compute the shot direction.
	 */
	void Fire(const FVector& TargetLocation);

	/** Skeletal mesh displayed by this weapon. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	/** Ability System Component belonging to the agent that owns this weapon. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> OwnerAbilitySystemComponent;

protected:
	/** Runtime startup hook reserved for future weapon initialization. */
	virtual void BeginPlay() override;

	/** Customization data currently applied to this weapon. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWeaponCustomization> ActiveWeaponCustomization;

};
