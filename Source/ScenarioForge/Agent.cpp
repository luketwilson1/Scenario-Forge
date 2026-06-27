// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Agent.cpp
 * @brief Implements the simulated agent character.
 */

#include "Agent.h"

#include "AbilitySystemComponent.h"
#include "AgentAIController.h"
#include "AgentCustomization.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DecisionComponent.h"
#include "GameplayAbilitySystem/AttributeSets/AgentAttributeSet.h"
#include "GameplayAbilitySystem/GameplayEffects/GE_Damage.h"
#include "ScenarioForgeGameplayTags.h"
#include "Weapon.h"
#include "WeaponCustomization.h"

/**
 * @brief Initializes components and default placement behavior for agent instances.
 */
AAgent::AAgent()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	AgentAttributeSet = CreateDefaultSubobject<UAgentAttributeSet>(TEXT("AgentAttributeSet"));

	// TODO: Temporary mesh import alignment fix. Move this into the mesh import/Blueprint setup.
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	AIControllerClass = AAgentAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

/**
 * @brief Gets the ability system component used by GAS.
 *
 * @return Ability system component owned by the agent.
 */
UAbilitySystemComponent* AAgent::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

/**
 * @brief Gets the data asset assigned to configure this agent.
 *
 * @return Assigned agent customization asset.
 */
UAgentCustomization* AAgent::GetAgentCustomization() const
{
	return AgentCustomization;
}

/**
 * @brief Gets the primary weapon currently equipped by this agent.
 *
 * @return Spawned primary weapon, or nullptr when no weapon is equipped.
 */
AWeapon* AAgent::GetPrimaryWeapon() const
{
	return PrimaryWeapon;
}

/**
 * @brief Applies construction-time customization after editable properties change.
 *
 * @param Transform Actor transform supplied by the construction call.
 */
void AAgent::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyAgentCustomization();
}

/**
 * @brief Initializes runtime GAS state, customization, and starting equipment.
 */
void AAgent::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AgentAttributeSet->InitMaxHealth(100.0f);
	AgentAttributeSet->InitHealth(AgentAttributeSet->GetMaxHealth());

	ApplyAgentCustomization();
	SpawnStartingWeapon();
}

/**
 * @brief Applies damage to this agent through the reusable GE_Damage gameplay effect.
 *
 * @param DamageAmount Positive damage amount requested by the damage source.
 * @param DamageSource Actor that caused the damage.
 */
void AAgent::ApplyDamage(float DamageAmount, AActor* DamageSource)
{
	if (bIsDead || DamageAmount <= 0.0f || !AbilitySystemComponent || !AgentAttributeSet)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(DamageSource);

	/** Build an instant damage spec that reads its magnitude from Data.Damage. */
	FGameplayEffectSpecHandle DamageSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		UGE_Damage::StaticClass(),
		1.0f,
		EffectContext);

	if (!DamageSpecHandle.IsValid())
	{
		return;
	}

	/** Damage is stored as a negative additive value so GE_Damage subtracts health. */
	DamageSpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage.GetTag(), -DamageAmount);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());

	if (AgentAttributeSet->GetHealth() <= 0.0f)
	{
		HandleDeath();
	}
}

/**
 * @brief Marks the agent dead and records State.Dead in its decision state.
 */
void AAgent::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	if (AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetController()))
	{
		if (UDecisionComponent* DecisionComponent = AgentAIController->GetDecisionComponent())
		{
			DecisionComponent->ShutdownDecisionMaking(TAG_State_Dead.GetTag());
		}
	}
}

/**
 * @brief Applies visual customization data from the assigned agent sheet.
 */
void AAgent::ApplyAgentCustomization()
{
	if (!AgentCustomization)
	{
		return;
	}

	if (USkeletalMesh* SkeletalMesh = AgentCustomization->Appearance.SkeletalMesh)
	{
		GetMesh()->SetSkeletalMesh(SkeletalMesh);
	}

	if (UClass* AnimationBlueprint = AgentCustomization->Appearance.AnimationBlueprint)
	{
		GetMesh()->SetAnimInstanceClass(AnimationBlueprint);
	}

	for (const FMaterialOverride& MaterialOverride : AgentCustomization->Appearance.MaterialOverrides)
	{
		if (MaterialOverride.Material)
		{
			GetMesh()->SetMaterial(MaterialOverride.MaterialSlotIndex, MaterialOverride.Material);
		}
	}
}

/**
 * @brief Spawns and configures the temporary starting weapon from the agent sheet.
 */
void AAgent::SpawnStartingWeapon()
{
	// TODO: Replace this with real equipment/loadout spawning and socket attachment rules.
	if (!StartingWeapon || PrimaryWeapon)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	PrimaryWeapon = World->SpawnActor<AWeapon>(AWeapon::StaticClass(), GetActorTransform(), SpawnParameters);
	if (!PrimaryWeapon)
	{
		return;
	}

	PrimaryWeapon->OwnerAbilitySystemComponent = AbilitySystemComponent;
	PrimaryWeapon->ApplyWeaponCustomization(StartingWeapon);
	AttachStartingWeapon();
}

/**
 * @brief Attaches the temporary starting weapon to the right hand socket.
 */
void AAgent::AttachStartingWeapon()
{
	// TODO: Replace this with real equipment socket rules and tuned weapon offsets.
	if (!PrimaryWeapon || !PrimaryWeapon->SkeletalMeshComponent || !StartingWeapon)
	{
		return;
	}

	static const FName RightHandSocketName(TEXT("RightHand"));

	USkeletalMeshComponent* AgentMesh = GetMesh();
	if (!AgentMesh || !AgentMesh->DoesSocketExist(RightHandSocketName))
	{
		return;
	}

	PrimaryWeapon->AttachToComponent(AgentMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, RightHandSocketName);
}

/**
 * @brief Per-frame update hook for future agent runtime behavior.
 *
 * @param DeltaTime Seconds elapsed since the previous frame.
 */
void AAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

/**
 * @brief Binds player input when this character is possessed by a player.
 *
 * @param PlayerInputComponent Input component supplied by Unreal.
 */
void AAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

