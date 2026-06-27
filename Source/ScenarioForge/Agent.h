// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Agent.h
 * @brief Declares the character actor used by simulated agents.
 */

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Agent.generated.h"

class UAbilitySystemComponent;
class UAgentAttributeSet;
class AWeapon;
class UAgentCustomization;
class UWeaponCustomization;

/**
 * @brief Character actor that owns agent visuals, abilities, attributes, and starting equipment.
 */
UCLASS()
class SCENARIOFORGE_API AAgent : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/** Initializes the agent character, ability system, mesh alignment, and AI possession defaults. */
	AAgent();

	/**
	 * @brief Gets the ability system component owned by this agent.
	 *
	 * @return The agent's ability system component.
	 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/**
	 * @brief Gets the customization asset assigned to this agent instance.
	 *
	 * @return The customization asset used to configure this agent, or nullptr when none is assigned.
	 */
	UAgentCustomization* GetAgentCustomization() const;

	/**
	 * @brief Gets the primary weapon currently equipped by this agent.
	 *
	 * @return Spawned primary weapon, or nullptr when the agent has no weapon.
	 */
	AWeapon* GetPrimaryWeapon() const;

	/**
	 * @brief Applies incoming damage through the agent's Gameplay Ability System attributes.
	 *
	 * @param DamageAmount Positive damage amount to apply.
	 * @param DamageSource Actor responsible for the damage application.
	 */
	void ApplyDamage(float DamageAmount, AActor* DamageSource);

protected:
	/**
	 * @brief Reapplies editor-visible customization whenever construction data changes.
	 *
	 * @param Transform Construction transform supplied by Unreal.
	 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Initializes runtime ability data, applies customization, and spawns starting equipment. */
	virtual void BeginPlay() override;

	/** Applies mesh, animation, and material data from the assigned customization asset. */
	void ApplyAgentCustomization();

	/** Spawns the temporary starting weapon defined on this agent. */
	void SpawnStartingWeapon();

	/** Attaches the spawned starting weapon to the agent's hand socket. */
	void AttachStartingWeapon();

	/** Manages the agent's granted abilities, active effects, attributes, and owned gameplay tags. */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** Runtime attributes owned by this agent's ability system. */
	UPROPERTY()
	TObjectPtr<UAgentAttributeSet> AgentAttributeSet;

	/** Primary weapon currently spawned and attached to this agent. */
	UPROPERTY()
	TObjectPtr<AWeapon> PrimaryWeapon;

	/** True after this agent has reached zero health and processed death once. */
	bool bIsDead = false;

	/** Marks this agent as dead and updates its decision state. */
	void HandleDeath();

	/** Data asset used to configure this placed agent instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent")
	TObjectPtr<UAgentCustomization> AgentCustomization;

	/** TODO TRAFFIC CONE: Temporary bootstrapped weapon setup until loadout/equipment data is designed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Weapons")
	TObjectPtr<UWeaponCustomization> StartingWeapon;

public:
	/**
	 * @brief Per-frame update hook for future agent runtime behavior.
	 *
	 * @param DeltaTime Seconds elapsed since the previous frame.
	 */
	virtual void Tick(float DeltaTime) override;

	/**
	 * @brief Binds input actions when this agent is possessed by a player controller.
	 *
	 * @param PlayerInputComponent Input component receiving bindings.
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
