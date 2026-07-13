// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GA_ThrowGrenade.h
 * @brief Declares the gameplay ability used to receive AI grenade throw data.
 */

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayTagContainer.h"
#include "GA_ThrowGrenade.generated.h"

class UAbilityTask_WaitGameplayEvent;

/**
 * @brief Gameplay ability used to play grenade throw animation and release the grenade from an animation event.
 */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UGA_ThrowGrenade : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** Initializes the ability as per-agent instanced so pending launch data is not shared. */
	UGA_ThrowGrenade();

	/** Stores the AI-calculated launch velocity for the next grenade activation. */
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	void SetPendingLaunchVelocity(const FVector& InLaunchVelocity);

	/** Clears any pending AI-calculated launch velocity. */
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	void ClearPendingLaunchVelocity();

	/** Gets whether an AI-calculated launch velocity is pending. */
	UFUNCTION(BlueprintPure, Category = "Grenade")
	bool HasPendingLaunchVelocity() const { return bHasPendingLaunchVelocity; }

	/** Gets the pending AI-calculated launch velocity. */
	UFUNCTION(BlueprintPure, Category = "Grenade")
	FVector GetPendingLaunchVelocity() const { return PendingLaunchVelocity; }

	/** Gets the release transform from the avatar pawn's grenade release socket. */
	UFUNCTION(BlueprintPure, Category = "Grenade")
	FTransform GetGrenadeReleaseTransform() const;

protected:
	/** Activates the throw montage and waits for the animation release event. */
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/** Called when the throw montage reaches its grenade release event. Override in Blueprint to spawn the grenade. */
	UFUNCTION(BlueprintNativeEvent, Category = "Grenade")
	void HandleGrenadeRelease(const FGameplayEventData& EventData);
	virtual void HandleGrenadeRelease_Implementation(const FGameplayEventData& EventData);

	/** Applies the AI grenade throw cooldown tag using the current grenade properties. */
	void ApplyGrenadeThrowCooldown(const FGameplayAbilityActorInfo* ActorInfo) const;

	/** Ends the ability when the montage completes, blends out, is interrupted, or is cancelled. */
	UFUNCTION()
	void HandleThrowMontageFinished();

	/** Handles the gameplay event fired by the throw montage release notify. */
	UFUNCTION()
	void HandleThrowReleaseEvent(FGameplayEventData Payload);

	/** Pending world-space launch velocity supplied by AI grenade action behavior. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	FVector PendingLaunchVelocity = FVector::ZeroVector;

	/** True when PendingLaunchVelocity contains valid throw data. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	bool bHasPendingLaunchVelocity = false;

	/** Event tag the throw montage notify sends at the grenade release frame. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Animation")
	FGameplayTag ThrowReleaseEventTag;

	/** Active wait task for the throw release animation event. */
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ThrowReleaseEventTask;

	/** True after the montage sends the grenade release gameplay event. */
	bool bReceivedThrowReleaseEvent = false;

	/** True after a grenade projectile has spawned during this activation. */
	bool bSpawnedGrenadeProjectile = false;
};
