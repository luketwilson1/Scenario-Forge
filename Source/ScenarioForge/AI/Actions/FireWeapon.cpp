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
	AddedEffects.AddTag(TAG_State_CurrentTargetDead.GetTag());
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

	/** Ask the weapon to handle repeated muzzle-forward shots and fire-rate timing for the selected burst window. */
	PrimaryWeapon->FireBurst(BurstDuration);

	UAbilitySystemComponent* AbilitySystemComponent = OwningAgent->GetAbilitySystemComponent();
	if (!AbilitySystemComponent || BurstSeparation <= 0.0f)
	{
		return EActionResult::Succeeded;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(OwningAgent);

	FGameplayEffectSpecHandle BurstSeparationSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		UGE_BurstSeparation::StaticClass(),
		1.0f,
		EffectContext);

	if (!BurstSeparationSpecHandle.IsValid())
	{
		return EActionResult::Succeeded;
	}

	BurstSeparationSpecHandle.Data->SetDuration(BurstSeparation, true);
	BurstSeparationSpecHandle.Data->DynamicGrantedTags.AddTag(TAG_State_Weapon_BurstSeparation.GetTag());
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*BurstSeparationSpecHandle.Data.Get());
	return EActionResult::Succeeded;
}
