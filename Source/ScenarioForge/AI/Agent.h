// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AI/Agent.h
 * @brief Declares the pawn actor used by simulated agents.
 */

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "../FactionTypes.h"
#include "../GrenadeTypes.h"
#include "Agent.generated.h"

class UAbilitySystemComponent;
class UAgentAttributeSet;
class UPrimitiveComponent;
class USkeletalMeshComponent;
class USphereComponent;
class AWeapon;
class AProjectile;
class UAgentSheet;
class UEquipmentComponent;
class UPawnSheet;
class UWeaponSheet;
struct FOnAttributeChangeData;

/**
 * @brief Character actor that owns agent visuals, movement, abilities, attributes, and starting equipment.
 */
UCLASS()
class SCENARIOFORGE_API AAgent : public ACharacter, public IAbilitySystemInterface
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
	 * @brief Gets the sheet asset assigned to this agent instance.
	 *
	 * @return The sheet asset used to configure this agent, or nullptr when none is assigned.
	 */
	UAgentSheet* GetAgentSheet() const;

	/** Gets this placed agent's faction override or its agent sheet's resolved faction. */
	UFUNCTION(BlueprintPure, Category = "Agent|Faction")
	EFaction GetResolvedFaction() const;

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

	/** Gets the resolved pawn presentation sheet assigned through this agent's sheet. */
	const UPawnSheet* GetResolvedPawnSheet() const;

	/** Gets the transform used as the release point for thrown grenades. */
	UFUNCTION(BlueprintCallable, Category = "Agent|Animation")
	FTransform GetGrenadeReleaseTransform() const;

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

	/** Returns whether this agent is incapacitated by its configured health threshold. */
	UFUNCTION(BlueprintPure, Category = "Agent|State")
	bool IsDowned() const { return bIsDowned; }

	/** Returns whether this agent is currently exposing itself to peek from cover. */
	UFUNCTION(BlueprintPure, Category = "Agent|Cover")
	bool IsPeekingFromCover() const { return bIsPeekingFromCover; }

	/** Updates the animation-facing cover peek state. */
	void SetPeekingFromCover(bool bNewPeekingFromCover) { bIsPeekingFromCover = bNewPeekingFromCover; }

	/** Prevents path following from rotating the pawn toward a movement destination. */
	void DisableAutomaticMovementFacing();

	/** Stores the most recent danger source location this agent should dodge away from. */
	void SetCurrentDangerSourceLocation(const FVector& DangerLocation);

	/** Gets the most recent danger source location when one is known. */
	bool GetCurrentDangerSourceLocation(FVector& OutDangerLocation) const;

	/** Clears the remembered danger source location. */
	void ClearCurrentDangerSourceLocation();

	/** Registers an explosive projectile whose danger volume currently overlaps this agent. */
	void AddGrenadeDangerSource(AProjectile* DangerSource);

	/** Unregisters an explosive projectile that no longer threatens this agent. */
	void RemoveGrenadeDangerSource(AProjectile* DangerSource);

	/** Gets all still-valid explosive projectiles currently threatening this agent. */
	void GetGrenadeDangerSources(TArray<AActor*>& OutDangerSources) const;

	/** Returns whether at least one valid explosive danger source remains. */
	bool HasGrenadeDangerSources() const;

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
	 * @brief Reapplies editor-visible sheet whenever construction data changes.
	 *
	 * @param Transform Construction transform supplied by Unreal.
	 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Initializes runtime ability data, applies sheet, and spawns starting equipment. */
	virtual void BeginPlay() override;

	/** Clears fired-upon state timing when this agent leaves play. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Applies mesh, animation, and material data from the assigned sheet asset. */
	void ApplyAgentSheet();

	/** Applies the resolved nearby-bullet detection radius and collision state. */
	void ConfigureFiredUponDetection();

	/** Removes State.FiredUpon after no hostile bullet has refreshed it within the configured duration. */
	void ExpireFiredUponState();

	/**
	 * @brief Handles a projectile entering the agent's nearby-fire sphere.
	 *
	 * @param OverlappedComponent Detection sphere receiving the overlap.
	 * @param OtherActor Actor entering the detection sphere.
	 * @param OtherComponent Primitive component that entered the sphere.
	 * @param OtherBodyIndex Body index supplied by the overlap event.
	 * @param bFromSweep True when the overlap came from swept movement.
	 * @param SweepResult Sweep hit data supplied by Unreal.
	 */
	UFUNCTION()
	void HandleFiredUponBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	/** Spawns the configured starting weapons defined on this agent. */
	void SpawnStartingWeapons();

	/** Applies starting equipment counts from the assigned agent sheet. */
	void InitializeStartingEquipment();

	/** Gets the primary weapon sheet after resolving placed-agent overrides and agent sheet defaults. */
	UWeaponSheet* GetResolvedPrimaryWeaponSheet() const;

	/** Gets the secondary weapon sheet after resolving placed-agent overrides and agent sheet defaults. */
	UWeaponSheet* GetResolvedSecondaryWeaponSheet() const;

	/** Attaches a spawned weapon actor to the named mesh socket. */
	void AttachWeaponToSocket(AWeapon* Weapon, FName SocketName);

	/** Updates body turning and animation-facing aim offsets toward a known target location. */
	void UpdateCurrentAimToTarget(const FVector& TargetLocation, bool bHasTargetLocation, float DeltaTime);

	/** Manages the agent's granted abilities, active effects, attributes, and owned gameplay tags. */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** Runtime attributes owned by this agent's ability system. */
	UPROPERTY()
	TObjectPtr<UAgentAttributeSet> AgentAttributeSet;

	/** Stores equipment counts and current held equipment selections for this agent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Components")
	TObjectPtr<UEquipmentComponent> EquipmentComponent;

	/** Spherical query volume that detects hostile bullets passing near this agent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Components")
	TObjectPtr<USphereComponent> FiredUponDetectionComponent;

	/** Timer refreshed by nearby hostile bullets before State.FiredUpon expires. */
	FTimerHandle FiredUponStateTimerHandle;

	/** True after this agent has reached zero health and processed death once. */
	bool bIsDead = false;

	/** True while health is at or below the configured downed threshold. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Agent|State")
	bool bIsDowned = false;

	/** Subscription used to evaluate death, downing, and recovery for every GAS health change. */
	FDelegateHandle HealthChangedDelegateHandle;

	/** Most recent danger source location detected for this agent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Dodge")
	FVector CurrentDangerSourceLocation = FVector::ZeroVector;

	/** True when CurrentDangerSourceLocation contains a valid remembered location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Dodge")
	bool bHasCurrentDangerSourceLocation = false;

	/** Explosive projectiles whose danger volumes currently contain this agent. */
	TSet<TWeakObjectPtr<AProjectile>> GrenadeDangerSources;

	/** World-space dodge direction selected by the current dodge behavior. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Dodge")
	FVector DodgeDirectionWorld = FVector::ZeroVector;

	/** Local-space dodge direction selected by the current dodge behavior. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent|Dodge")
	FVector DodgeDirectionLocal = FVector::ZeroVector;

	/** Runtime aim offset yaw value for animation use. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Agent|Aiming")
	float CurrentAimYaw = 0.0f;

	/** Runtime aim offset pitch value for animation use. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Agent|Aiming")
	float CurrentAimPitch = 0.0f;

	/** Whether the agent is currently playing an exposed cover-peek pose. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Agent|Cover")
	bool bIsPeekingFromCover = false;

	/** World yaw selected for the current configured body-turn step. */
	float BodyTurnTargetYaw = 0.0f;

	/** True while the agent is rotating toward BodyTurnTargetYaw. */
	bool bBodyTurnInProgress = false;

	/** Marks this agent as dead and updates its decision state. */
	void HandleDeath();

	/** Responds to damage and healing applied through the Health gameplay attribute. */
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);

	/** Evaluates the configured downed threshold against current health. */
	void EvaluateDownedState();

	/** Applies or removes the reversible incapacitated state. */
	void SetDowned(bool bNewDowned);

	/** Registers animation-facing state replicated by the agent pawn. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Data asset used to configure this placed agent instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent", meta = (DisplayName = "Agent Sheet"))
	TObjectPtr<UAgentSheet> AgentSheet;

	/** Whether this placed agent overrides the faction inherited from its agent sheet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Faction", meta = (DisplayName = "Override Faction"))
	bool bOverrideFaction = false;

	/** Faction used by this placed agent when the instance override is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Faction", meta = (EditCondition = "bOverrideFaction"))
	EFaction Faction = EFaction::Red;

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
	TObjectPtr<UWeaponSheet> PrimaryWeapon;

	/** Whether this placed agent overrides the secondary weapon from its agent sheet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Equipment", meta = (InlineEditConditionToggle))
	bool bOverrideSecondaryWeapon = false;

	/** Secondary weapon override for this placed agent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent|Equipment", meta = (EditCondition = "bOverrideSecondaryWeapon", DisplayName = "Secondary Weapon"))
	TObjectPtr<UWeaponSheet> SecondaryWeapon;

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
