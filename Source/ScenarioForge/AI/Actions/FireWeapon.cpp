// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FireWeapon.cpp
 * @brief Implements the GOAP action that fires an equipped weapon at the current enemy target.
 */

#include "FireWeapon.h"

#include "Agent.h"
#include "AgentAIController.h"
#include "AgentCustomization.h"
#include "AbilitySystemComponent.h"
#include "Planner.h"
#include "GameplayAbilitySystem/GameplayEffects/GE_BurstSeparation.h"
#include "ScenarioForgeGameplayTags.h"
#include "Weapon.h"
#include "WeaponCustomization.h"

/**
 * @brief Configures the fire action's planning preconditions and effects.
 */
UFireWeapon::UFireWeapon()
{
	TruePreconditions.AddTag(TAG_State_SeesEnemy.GetTag());
	FalsePreconditions.AddTag(TAG_State_Dead.GetTag());
	FalsePreconditions.AddTag(TAG_State_Weapon_BurstSeparation.GetTag());
	AddedEffects.AddTag(TAG_State_DestroyTarget.GetTag());
}

/**
 * @brief Fires the owner agent's primary weapon at the controller's current enemy target.
 *
 * @param Planner Planner executing the fire weapon action.
 * @return Succeeded after firing, Failed when firing requirements are missing, or Invalid without a planner.
 */
EActionResult UFireWeapon::Execute(UPlanner* Planner)
{
	if (!Planner)
	{
		return EActionResult::Invalid;
	}

	/** Resolve the controller, pawn, target, and equipped weapon required to shoot. */
	AAgentAIController* AgentAIController = Cast<AAgentAIController>(Planner->GetOwner());
	AAgent* OwningAgent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	AActor* TargetActor = AgentAIController ? AgentAIController->GetCurrentEnemyTarget() : nullptr;
	AWeapon* PrimaryWeapon = OwningAgent ? OwningAgent->GetPrimaryWeapon() : nullptr;
	UAgentCustomization* AgentCustomization = OwningAgent ? OwningAgent->GetAgentCustomization() : nullptr;

	if (!PrimaryWeapon || !TargetActor || !AgentCustomization)
	{
		return EActionResult::Failed;
	}

	UWeaponCustomization* WeaponCustomization = PrimaryWeapon->GetActiveWeaponCustomization();
	const FWeaponProperties* WeaponProperties = WeaponCustomization
		? AgentCustomization->FindResolvedWeaponProperties(WeaponCustomization)
		: nullptr;

	const float MinimumBurstDuration = WeaponProperties ? WeaponProperties->MinimumBurstDuration : 0.0f;
	const float MaximumBurstDuration = WeaponProperties ? WeaponProperties->MaximumBurstDuration : 0.0f;
	const float BurstDuration = FMath::FRandRange(
		FMath::Min(MinimumBurstDuration, MaximumBurstDuration),
		FMath::Max(MinimumBurstDuration, MaximumBurstDuration));
	const float MinimumBurstSeparation = WeaponProperties ? WeaponProperties->MinimumBurstSeparation : 0.0f;
	const float MaximumBurstSeparation = WeaponProperties ? WeaponProperties->MaximumBurstSeparation : 0.0f;
	const float BurstSeparation = FMath::FRandRange(
		FMath::Min(MinimumBurstSeparation, MaximumBurstSeparation),
		FMath::Max(MinimumBurstSeparation, MaximumBurstSeparation));

	const bool bTimedBurst = BurstDuration > 0.0f && WeaponCustomization && WeaponCustomization->RateOfFire > 0;
	if (bTimedBurst)
	{
		const TWeakObjectPtr<UPlanner> WeakPlanner = Planner;
		FOnWeaponBurstFinished BurstFinishedDelegate;
		BurstFinishedDelegate.BindLambda([WeakPlanner]()
		{
			if (UPlanner* ResolvedPlanner = WeakPlanner.Get())
			{
				ResolvedPlanner->CompleteActiveAction(EActionResult::Succeeded);
			}
		});

		/** Fire the timed burst and retain the planner lock until its timer completes. */
		PrimaryWeapon->FireBurst(BurstDuration, MoveTemp(BurstFinishedDelegate));
	}
	else
	{
		/** A zero-duration burst is a single synchronous shot. */
		PrimaryWeapon->FireBurst(BurstDuration);
	}

	UAbilitySystemComponent* AbilitySystemComponent = OwningAgent->GetAbilitySystemComponent();
	if (!AbilitySystemComponent || BurstSeparation <= 0.0f)
	{
		return bTimedBurst ? EActionResult::Running : EActionResult::Succeeded;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(OwningAgent);

	FGameplayEffectSpecHandle BurstSeparationSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		UGE_BurstSeparation::StaticClass(),
		1.0f,
		EffectContext);

	if (!BurstSeparationSpecHandle.IsValid())
	{
		return bTimedBurst ? EActionResult::Running : EActionResult::Succeeded;
	}

	BurstSeparationSpecHandle.Data->SetDuration(BurstSeparation, true);
	BurstSeparationSpecHandle.Data->DynamicGrantedTags.AddTag(TAG_State_Weapon_BurstSeparation.GetTag());
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*BurstSeparationSpecHandle.Data.Get());
	return bTimedBurst ? EActionResult::Running : EActionResult::Succeeded;
}

/**
 * @brief Stops the owning agent's active burst before the planner replaces this action.
 *
 * @param Planner Planner requesting interruption.
 * @return True when an equipped primary weapon was found and stopped.
 */
bool UFireWeapon::Interrupt(UPlanner* Planner)
{
	AAgentAIController* AgentAIController = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	AAgent* OwningAgent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	AWeapon* PrimaryWeapon = OwningAgent ? OwningAgent->GetPrimaryWeapon() : nullptr;
	if (!PrimaryWeapon)
	{
		return false;
	}

	PrimaryWeapon->StopFireBurst();
	return true;
}
