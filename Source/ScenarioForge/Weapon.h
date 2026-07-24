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
class UWeaponSheet;

/** Callback invoked when a timed weapon burst finishes. */
DECLARE_DELEGATE(FOnWeaponBurstFinished);

/**
 * @brief Runtime weapon actor that applies weapon sheet and fires projectiles.
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
	 * @param WeaponSheet Sheet data to apply to this weapon.
	 */
	void ApplyWeaponSheet(UWeaponSheet* WeaponSheet);

	/**
	 * @brief Gets the sheet sheet currently applied to this runtime weapon.
	 *
	 * @return Active weapon sheet sheet, or nullptr when none has been applied.
	 */
	UWeaponSheet* GetActiveWeaponSheet() const;

	/**
	 * @brief Gets the current world transform used as this weapon's muzzle.
	 *
	 * @return Muzzle socket world transform, or the weapon transform when no muzzle socket is available.
	 */
	FTransform GetMuzzleTransform() const;

	/**
	 * @brief Fires one projectile from the muzzle toward an actor when the muzzle is visually aligned.
	 *
	 * @param TargetActor Actor whose current location supplies the desired trajectory.
	 * @param AllowableAimDeviation Maximum permitted muzzle-to-target angle in degrees.
	 * @return True when a projectile was fired.
	 */
	bool FireAtTarget(const AActor* TargetActor, float AllowableAimDeviation);

	/** Returns whether the muzzle currently lies within the permitted angle of the target direction. */
	bool IsMuzzleAlignedWithTarget(const AActor* TargetActor, float AllowableAimDeviation) const;

	/**
	 * @brief Fires repeatedly from the muzzle for a limited burst duration.
	 *
	 * @param TargetActor Actor tracked throughout the burst.
	 * @param AllowableAimDeviation Maximum permitted muzzle-to-target angle for every shot.
	 * @param BurstDuration Seconds the burst should continue firing.
	 * @param OnFinished Callback invoked after a timed burst finishes.
	 */
	void FireBurst(
		AActor* TargetActor,
		float AllowableAimDeviation,
		float BurstDuration,
		FOnWeaponBurstFinished OnFinished = FOnWeaponBurstFinished());

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

	/** Sheet data currently applied to this weapon. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWeaponSheet> ActiveWeaponSheet;

	/** Timer that drives repeated shots during a burst. */
	FTimerHandle BurstFireTimerHandle;

	/** Timer that stops the current burst after its configured duration. */
	FTimerHandle BurstStopTimerHandle;

	/** Callback retained until the current timed burst finishes. */
	FOnWeaponBurstFinished BurstFinishedDelegate;

	/** Actor whose current location is sampled for every shot in the active burst. */
	TWeakObjectPtr<AActor> BurstTargetActor;

	/** Maximum muzzle deviation allowed for each shot in the active burst. */
	float BurstAllowableAimDeviation = 15.0f;

	/** Fires one burst shot toward the tracked target when visually aligned. */
	void HandleBurstShot();

	/** Stops the current timed burst and notifies its completion callback. */
	void FinishFireBurst();

};
