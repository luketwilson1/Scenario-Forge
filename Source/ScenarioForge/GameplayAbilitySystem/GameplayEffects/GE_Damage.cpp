// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GE_Damage.cpp
 * @brief Implements the instant SetByCaller damage Gameplay Effect.
 */

#include "GE_Damage.h"

#include "../AttributeSets/AgentAttributeSet.h"
#include "../../ScenarioForgeGameplayTags.h"

/**
 * @brief Builds a reusable instant effect that adds Data.Damage to Health.
 */
UGE_Damage::UGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DamageMagnitude;
	DamageMagnitude.DataTag = TAG_Data_Damage.GetTag();

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = UAgentAttributeSet::GetHealthAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);

	Modifiers.Add(DamageModifier);
}
