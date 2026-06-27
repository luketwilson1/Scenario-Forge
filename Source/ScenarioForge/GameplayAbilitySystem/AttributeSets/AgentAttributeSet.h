// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentAttributeSet.h
 * @brief Declares Gameplay Ability System attributes used by agents.
 */

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AgentAttributeSet.generated.h"

/**
 * @brief Generates standard GAS property, getter, setter, and initializer helpers for an attribute.
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * @brief Attribute set containing health values for simulated agents.
 */
UCLASS()
class SCENARIOFORGE_API UAgentAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	
	/** Current health value used to determine whether the agent is alive. */
	UPROPERTY()
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, Health)


	/** Maximum health value used when initializing and clamping health later. */
	UPROPERTY()
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UAgentAttributeSet, MaxHealth)
};
