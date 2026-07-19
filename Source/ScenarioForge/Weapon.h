// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Weapon.h
 * @brief Declares the runtime weapon actor used by agents.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Weapon.generated.h"

class UAbilitySystemComponent;
class USkeletalMeshComponent;
class UWeaponCustomization;

/** Callback invoked when a timed weapon burst finishes. */
DECLARE_DELEGATE(FOnWeaponBurstFinished);

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
	 * @brief Gets the customization sheet currently applied to this runtime weapon.
	 *
	 * @return Active weapon customization sheet, or nullptr when none has been applied.
	 */
	UWeaponCustomization* GetActiveWeaponCustomization() const;

	/**
	 * @brief Gets the current world transform used as this weapon's muzzle.
	 *
	 * @return Muzzle socket world transform, or the weapon transform when no muzzle socket is available.
	 */
	FTransform GetMuzzleTransform() const;

	/**
	 * @brief Fires one projectile straight forward from the muzzle socket.
	 *
	 * @param TargetLocation Unused target point kept for temporary call-site compatibility.
	 */
	void Fire(const FVector& TargetLocation);

	/**
	 * @brief Fires repeatedly from the muzzle for a limited burst duration.
	 *
	 * @param BurstDuration Seconds the burst should continue firing.
	 * @param OnFinished Callback invoked after a timed burst finishes.
	 */
	void FireBurst(float BurstDuration, FOnWeaponBurstFinished OnFinished = FOnWeaponBurstFinished());

	/** Stops any active burst fire timer. */
	void StopFireBurst();

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

	/** Timer that drives repeated shots during a burst. */
	FTimerHandle BurstFireTimerHandle;

	/** Timer that stops the current burst after its configured duration. */
	FTimerHandle BurstStopTimerHandle;

	/** Callback retained until the current timed burst finishes. */
	FOnWeaponBurstFinished BurstFinishedDelegate;

	/** Fires one burst shot straight forward from the muzzle. */
	void HandleBurstShot();

	/** Stops the current timed burst and notifies its completion callback. */
	void FinishFireBurst();

};
