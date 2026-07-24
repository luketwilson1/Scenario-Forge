// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file WeaponFiringComponent.h
 * @brief Declares the controller-owned weapon execution channel that can run beside GOAP movement.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponFiringComponent.generated.h"

class AAgent;
class AAgentAIController;
class AWeapon;
class UPlanner;

/** Runs deliberate or action-permitted opportunistic weapon bursts independently of locomotion actions. */
UCLASS(ClassGroup = (AI))
class SCENARIOFORGE_API UWeaponFiringComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponFiringComponent();

	/** Starts a planner-owned deliberate attack request. Completion is reported back to the planner. */
	bool BeginDeliberateFire(UPlanner* Planner);

	/** Cancels a deliberate request when its FireWeapon action is interrupted. */
	bool CancelDeliberateFire(const UPlanner* Planner);

	/** Stops every pending alignment check and active burst without completing a planner action. */
	void StopAllFiring();

protected:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Returns the owning controller and its possessed agent. */
	bool ResolveOwner(AAgentAIController*& OutController, AAgent*& OutAgent) const;

	/** Checks shared visible-target, range, death, cooldown, weapon, and sheet requirements. */
	bool ResolveFireContext(
		AActor*& OutTarget,
		AAgent*& OutAgent,
		AWeapon*& OutWeapon,
		bool bIgnoreBurstSeparation = false) const;

	/** Returns whether the active GOAP action permits an independent visible-target burst. */
	bool ShouldFireOpportunistically() const;

	/** Starts one configured burst and applies its burst-separation gameplay effect. */
	bool StartBurst(AActor* Target, AAgent* Agent, AWeapon* Weapon, bool bDeliberate);

	/** Applies the configured interval before another burst may begin. */
	void ApplyBurstSeparation(AAgent* Agent, float BurstSeparation);

	/** Receives completion from AWeapon after a timed burst. */
	void HandleBurstFinished();

	/** Completes the deliberate FireWeapon action on the next safe component update. */
	void CompleteDeliberateFire(bool bSucceeded);

	/** Stops the current weapon burst and clears component-owned burst state. */
	void StopActiveBurst();

	/** Planner whose FireWeapon action is waiting for this channel. */
	TWeakObjectPtr<UPlanner> DeliberatePlanner;

	/** Weapon currently executing a burst for this component. */
	TWeakObjectPtr<AWeapon> ActiveBurstWeapon;

	/** Absolute time at which a deliberate alignment wait fails. */
	double DeliberateAlignmentDeadline = 0.0;

	/** Earliest time an opportunistic burst may begin when separation is zero or very short. */
	double NextOpportunisticFireTime = 0.0;

	/** True while AWeapon owns a timed burst callback. */
	bool bBurstInProgress = false;

	/** True when the active burst belongs to the deliberate FireWeapon action. */
	bool bActiveBurstIsDeliberate = false;
};
