// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GA_Melee.h
 * @brief Declares the montage-driven melee gameplay ability.
 */

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Melee.generated.h"

class USkeletalMeshComponent;
class AAgent;

/** Plays the pawn's melee montage and applies damage from its melee trace notify state. */
UCLASS(Blueprintable)
class SCENARIOFORGE_API UGA_Melee : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Melee();

	/** Performs one sphere sweep between the configured mesh bones for the current swing. */
	void PerformMeleeTrace(USkeletalMeshComponent* MeshComponent, FName StartBoneName, FName EndBoneName, float SphereRadius);

protected:
	virtual void ActivateAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/** Completes a successful melee planner action when the montage finishes normally. */
	UFUNCTION()
	void HandleMeleeMontageCompleted();

	/** Releases a melee planner action when the montage is cancelled or interrupted. */
	UFUNCTION()
	void HandleMeleeMontageInterrupted();

	/** Agents already damaged during this ability activation. */
	TSet<TWeakObjectPtr<AAgent>> HitAgents;
};
