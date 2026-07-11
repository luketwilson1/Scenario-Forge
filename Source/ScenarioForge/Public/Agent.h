// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Agent.h
 * @brief Declares the pawn actor used by simulated agents.
 */

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "../GrenadeTypes.h"
#include "Agent.generated.h"

class UAbilitySystemComponent;
class UAgentAttributeSet;
class UCapsuleComponent;
class UFloatingPawnMovement;
class UPawnMovementComponent;
class USkeletalMeshComponent;
class AWeapon;
class UAgentCustomization;
class UEquipmentComponent;
class UPawnCustomization;
class UWeaponCustomization;

/**
 * @brief Pawn actor that owns agent visuals, movement, abilities, attributes, and starting equipment.
 */
UCLASS()
class SCENARIOFORGE_API AAgent : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/** Initializes the agent pawn, movement, ability system, mesh alignment, and AI possession defaults. */
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

	/** Gets the capsule used for collision and placement. */
	UCapsuleComponent* GetCapsuleComponent() const;

	/** Gets the skeletal mesh used for this agent's visuals and sockets. */
	USkeletalMeshComponent* GetMesh() const;

	/** Gets the movement component used by AI path following. */
	virtual UPawnMovementComponent* GetMovementComponent() const override;

	/** Assigns the designer-facing name used by the scenario editor. */
	void SetAgentName(const FString& InAgentName);

	/**
	 * @brief Gets the primary weapon currently equipped by this agent.
	 *
	 * @return Spawned primary weapon, or nullptr when the agent has no weapon.
	 */
	AWeapon* GetPrimaryWeapon() const;

	/**
	 * @brief Gets the component that stores equipment held by this agent.
	 *
	 * @return Equipment component owned by the agent.
	 */
	UEquipmentComponent* GetEquipmentComponent() const;

	/** Gets the resolved pawn presentation sheet assigned through this agent's customization. */
	const UPawnCustomization* GetResolvedPawnCustomization() const;

	/** Gets the transform used as the release point for thrown grenades. */
	UFUNCTION(BlueprintCallable, Category = "Agent|Animation")
	FTransform GetGrenadeReleaseTransform() const;

	/**
	 * @brief Aims this agent's equipped weapon toward another actor.
	 *
	 * @param TargetActor Actor to aim at.
	 */
	void AimAtActor(const AActor* TargetActor);

	/**
	 * @brief Applies incoming damage through the agent's Gameplay Ability System attributes.
	 *
	 * @param DamageAmount Positive damage amount to apply.
	 * @param DamageSource Actor responsible for the damage application.
	 */
	void ApplyDamage(float DamageAmount, AActor* DamageSource);

	/** Returns true once this agent has processed death. */
	UFUNCTION(BlueprintPure, Category = "Agent|State")
	bool IsDead() const;

	/** Stores the most recent danger source location this agent should dodge away from. */
	void SetCurrentDangerSourceLocation(const FVector& DangerLocation);

	/** Gets the most recent danger source location when one is known. */
	bool GetCurrentDangerSourceLocation(FVector& OutDangerLocation) const;

	/** Clears the remembered danger source location. */
	void ClearCurrentDangerSourceLocation();

	/** Stores the current dodge direction in world and local space for animation use. */
	void SetDodgeDirectionWorld(const FVector& Direction);

	/** Clears the animation-facing dodge direction values. */
	void ClearDodgeDirection();

	/** World-space direction selected by the active dodge behavior. */
	UFUNCTION(BlueprintPure, Category = "Agent|Dodge")
	FVector GetDodgeDirectionWorld() const;

	/** Local-space direction selected by the active dodge behavior. */
	UFUNCTION(BlueprintPure, Category = "Agent|Dodge")
	FVector GetDodgeDirectionLocal() const;

protected:
	/**
	 * @brief Reapplies editor-visible customization whenever construction data changes.
	 *
	 * @param Transform Construction transform supplied by Unreal.
	 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Initializes runtime ability data, applies customization, and spawns starting equipment. */
	virtual void BeginPlay() override;

	/** Grants gameplay abilities from the resolved agent sheet. */
	void GrantAgentAbilities();

	/** Applies mesh, animation, and material data from the assigned customization asset. */
	void ApplyAgentCustomization();

	/** Spawns the configured starting weapons defined on this agent. */
	void SpawnStartingWeapons();

	/** Applies starting equipment counts from the assigned agent sheet. */
	void InitializeStartingEquipment();

	/** Gets the primary weapon sheet after resolving placed-agent overrides and agent sheet defaults. */
	UWeaponCustomization* GetResolvedPrimaryWeaponCustomization() const;

	/** Gets the secondary weapon sheet after resolving placed-agent overrides and agent sheet defaults. */
	UWeaponCustomization* GetResolvedSecondaryWeaponCustomization() const;

	/** Attaches a spawned weapon actor to the named mesh socket. */
	void AttachWeaponToSocket(AWeapon* Weapon, FName SocketName);

	/** Manages the agent's granted abilities, active effects, attributes, and owned gameplay tags. */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** Runtime attributes owned by this agent's ability system. */
	UPROPERTY()
	TObjectPtr<UAgentAttributeSet> AgentAttributeSet;

	/** Collision root used by the agent pawn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/** Visual mesh used by the agent pawn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Components")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	/** Movement component used by AI MoveTo requests. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Components")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	/** Stores equipment counts and current held equipment selections for this agent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Components")
	TObjectPtr<UEquipmentComponent> EquipmentComponent;

	/** True after this agent has reached zero health and processed death once. */
	bool bIsDead = false;

	/** Most recent danger source location detected for this agent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Dodge")
	FVector CurrentDangerSourceLocation = FVector::ZeroVector;

	/** True when CurrentDangerSourceLocation contains a valid remembered location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Dodge")
	bool bHasCurrentDangerSourceLocation = false;

	/** World-space dodge direction selected by the current dodge behavior. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Dodge")
	FVector DodgeDirectionWorld = FVector::ZeroVector;

	/** Local-space dodge direction selected by the current dodge behavior. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Dodge")
	FVector DodgeDirectionLocal = FVector::ZeroVector;

	/** Marks this agent as dead and updates its decision state. */
	void HandleDeath();

	/** Data asset used to configure this placed agent instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent", meta = (DisplayName = "Character Type"))
	TObjectPtr<UAgentCustomization> AgentCustomization;

	/** Designer-facing name shown by the scenario editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent", meta = (DisplayName = "Name"))
	FString AgentName;

	/** Runtime weapon actor created from PrimaryWeapon. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Runtime")
	TObjectPtr<AWeapon> EquippedWeapon;

	/** Runtime weapon actor created from SecondaryWeapon. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Runtime")
	TObjectPtr<AWeapon> StowedWeapon;
	
	/** Whether this placed agent overrides the primary weapon from its agent sheet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Equipment", meta = (InlineEditConditionToggle))
	bool bOverridePrimaryWeapon = false;

	/** Primary weapon override for this placed agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Equipment", meta = (EditCondition = "bOverridePrimaryWeapon", DisplayName = "Primary Weapon"))
	TObjectPtr<UWeaponCustomization> PrimaryWeapon;

	/** Whether this placed agent overrides the secondary weapon from its agent sheet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Equipment", meta = (InlineEditConditionToggle))
	bool bOverrideSecondaryWeapon = false;

	/** Secondary weapon override for this placed agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Equipment", meta = (EditCondition = "bOverrideSecondaryWeapon", DisplayName = "Secondary Weapon"))
	TObjectPtr<UWeaponCustomization> SecondaryWeapon;

	/** Whether this placed agent overrides starting grenade counts from its agent sheet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Equipment", meta = (InlineEditConditionToggle))
	bool bOverrideStartingGrenades = false;

	/** Starting grenade count overrides for this placed agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Equipment", meta = (EditCondition = "bOverrideStartingGrenades", DisplayName = "Starting Grenades"))
	TArray<FStartingGrenade> StartingGrenades;

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
