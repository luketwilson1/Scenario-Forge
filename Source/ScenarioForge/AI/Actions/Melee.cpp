// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Melee.cpp
 * @brief Implements the GOAP melee action.
 */

#include "Melee.h"

#include "AbilitySystemComponent.h"
#include "Agent.h"
#include "AgentAIController.h"
#include "GameplayAbilitySpec.h"
#include "GameplayAbilitySystem/Abilities/GA_Melee.h"
#include "Planner.h"
#include "ScenarioForgeGameplayTags.h"

UMelee::UMelee()
{
	TruePreconditions.AddTag(TAG_State_SeesEnemy.GetTag());
	TruePreconditions.AddTag(TAG_State_InMeleeRange.GetTag());
	FalsePreconditions.AddTag(TAG_State_Dead.GetTag());
	AddedEffects.AddTag(TAG_State_DestroyTarget.GetTag());
}

EActionResult UMelee::Execute(UPlanner* Planner)
{
	AAgentAIController* Controller = Planner ? Cast<AAgentAIController>(Planner->GetOwner()) : nullptr;
	AAgent* Agent = Controller ? Cast<AAgent>(Controller->GetPawn()) : nullptr;
	UAbilitySystemComponent* AbilitySystemComponent = Agent ? Agent->GetAbilitySystemComponent() : nullptr;
	if (!Planner || !Controller || !Agent || !AbilitySystemComponent)
	{
		return EActionResult::Invalid;
	}

	for (FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability || !AbilitySpec.Ability->IsA(UGA_Melee::StaticClass()))
		{
			continue;
		}

		return AbilitySystemComponent->TryActivateAbility(AbilitySpec.Handle)
			? EActionResult::Running
			: EActionResult::Failed;
	}

	UE_LOG(LogTemp, Warning, TEXT("MeleeAction[%s]: UGA_Melee is not granted."), *GetNameSafe(Agent));
	return EActionResult::Failed;
}
