// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GA_ThrowGrenade.h
 * @brief Declares the gameplay ability used to receive AI grenade throw data.
 */

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_ThrowGrenade.generated.h"

/**
 * @brief Gameplay ability placeholder for throwing grenades.
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

protected:
	/** Pending world-space launch velocity supplied by AI grenade action behavior. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	FVector PendingLaunchVelocity = FVector::ZeroVector;

	/** True when PendingLaunchVelocity contains valid throw data. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	bool bHasPendingLaunchVelocity = false;
};
