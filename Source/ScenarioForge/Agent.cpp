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
#include "Planner.h"
#include "EquipmentComponent.h"
#include "EquipmentCustomization.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySystem/AttributeSets/AgentAttributeSet.h"
#include "GameplayAbilitySystem/GameplayEffects/GE_Damage.h"
#include "PawnCustomization.h"
#include "ScenarioForgeGameplayTags.h"
#include "Weapon.h"
#include "WeaponCustomization.h"

/**
 * @brief Initializes components and default placement behavior for agent instances.
 */
AAgent::AAgent()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCanEverAffectNavigation(false);

	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MaxAcceleration = 2048.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	AgentAttributeSet = CreateDefaultSubobject<UAgentAttributeSet>(TEXT("AgentAttributeSet"));
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));

	// TODO: Temporary mesh import alignment fix. Move this into the mesh import/Blueprint setup.
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	AIControllerClass = AAgentAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	SetCanAffectNavigationGeneration(false, true);
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
	return EquippedWeapon;
}

UEquipmentComponent* AAgent::GetEquipmentComponent() const
{
	return EquipmentComponent;
}

const UPawnCustomization* AAgent::GetResolvedPawnCustomization() const
{
	return AgentCustomization ? AgentCustomization->GetResolvedPawnCustomization() : nullptr;
}

FTransform AAgent::GetGrenadeReleaseTransform() const
{
	const UPawnCustomization* PawnCustomization = GetResolvedPawnCustomization();
	const FName SocketName = PawnCustomization ? PawnCustomization->GrenadeReleaseSocketName : NAME_None;
	const USkeletalMeshComponent* AgentMesh = GetMesh();
	if (AgentMesh && SocketName != NAME_None && AgentMesh->DoesSocketExist(SocketName))
	{
		return AgentMesh->GetSocketTransform(SocketName);
	}

	return GetActorTransform();
}

/**
 * @brief Assigns the designer-facing name used by the scenario editor.
 *
 * @param InAgentName New designer-facing agent name.
 */
void AAgent::SetAgentName(const FString& InAgentName)
{
	AgentName = InAgentName;

#if WITH_EDITOR
	SetActorLabel(AgentName);
#endif
}

/**
 * @brief Applies construction-time customization after editable properties change.
 *
 * @param Transform Actor transform supplied by the construction call.
 */
void AAgent::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	if (!AgentName.IsEmpty())
	{
		SetActorLabel(AgentName);
	}
#endif

	ApplyAgentCustomization();
}

/**
 * @brief Initializes runtime GAS state, customization, and starting equipment.
 */
void AAgent::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	const float InitialMaxHealth = AgentCustomization
		? FMath::Max(0.0f, AgentCustomization->GetResolvedVitalityProperties().MaxHealth)
		: 100.0f;
	AgentAttributeSet->InitMaxHealth(InitialMaxHealth);
	AgentAttributeSet->InitHealth(AgentAttributeSet->GetMaxHealth());

	ApplyAgentCustomization();
	InitializeStartingEquipment();
	SpawnStartingWeapons();
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

bool AAgent::IsDead() const
{
	return bIsDead;
}

void AAgent::SetCurrentDangerSourceLocation(const FVector& DangerLocation)
{
	CurrentDangerSourceLocation = DangerLocation;
	bHasCurrentDangerSourceLocation = true;
}

bool AAgent::GetCurrentDangerSourceLocation(FVector& OutDangerLocation) const
{
	if (!bHasCurrentDangerSourceLocation)
	{
		return false;
	}

	OutDangerLocation = CurrentDangerSourceLocation;
	return true;
}

void AAgent::ClearCurrentDangerSourceLocation()
{
	CurrentDangerSourceLocation = FVector::ZeroVector;
	bHasCurrentDangerSourceLocation = false;
}

void AAgent::SetDodgeDirectionWorld(const FVector& Direction)
{
	DodgeDirectionWorld = Direction.GetSafeNormal2D();
	DodgeDirectionLocal = GetActorTransform().InverseTransformVectorNoScale(DodgeDirectionWorld).GetSafeNormal2D();
}

void AAgent::ClearDodgeDirection()
{
	DodgeDirectionWorld = FVector::ZeroVector;
	DodgeDirectionLocal = FVector::ZeroVector;
}

FVector AAgent::GetDodgeDirectionWorld() const
{
	return DodgeDirectionWorld;
}

FVector AAgent::GetDodgeDirectionLocal() const
{
	return DodgeDirectionLocal;
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
		if (UPlanner* Planner = AgentAIController->GetPlanner())
		{
			Planner->ShutdownDecisionMaking(TAG_State_Dead.GetTag());
		}
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	if (StowedWeapon)
	{
		StowedWeapon->Destroy();
		StowedWeapon = nullptr;
	}

	Destroy();
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

	const FAppearance& ResolvedAppearance = AgentCustomization->GetResolvedAppearance();
	if (USkeletalMesh* SkeletalMesh = ResolvedAppearance.SkeletalMesh)
	{
		GetMesh()->SetSkeletalMesh(SkeletalMesh);
	}

	if (UClass* AnimationBlueprint = ResolvedAppearance.AnimationBlueprint)
	{
		GetMesh()->SetAnimInstanceClass(AnimationBlueprint);
	}

	for (const FMaterialOverride& MaterialOverride : ResolvedAppearance.MaterialOverrides)
	{
		if (MaterialOverride.Material)
		{
			GetMesh()->SetMaterial(MaterialOverride.MaterialSlotIndex, MaterialOverride.Material);
		}
	}
}

/**
 * @brief Applies starting equipment counts from this agent's customization sheet.
 */
void AAgent::InitializeStartingEquipment()
{
	if (!EquipmentComponent)
	{
		return;
	}

	EquipmentComponent->HeldGrenades.Reset();
	EquipmentComponent->HeldGrenadeEquipment.Reset();
	const TArray<FStartingGrenade>& ResolvedStartingGrenades = bOverrideStartingGrenades || !AgentCustomization
		? StartingGrenades
		: AgentCustomization->GetResolvedStartingGrenades();
	EGrenadeType InitialGrenadeType = EGrenadeType::None;
	for (const FStartingGrenade& GrenadeEntry : ResolvedStartingGrenades)
	{
		if (!GrenadeEntry.Equipment
			|| GrenadeEntry.Equipment->Category != EEquipmentCategory::Grenade
			|| GrenadeEntry.Equipment->GrenadeType == EGrenadeType::None)
		{
			continue;
		}

		const EGrenadeType GrenadeType = GrenadeEntry.Equipment->GrenadeType;
		if (GrenadeType != EGrenadeType::None && GrenadeEntry.Count > 0)
		{
			EquipmentComponent->HeldGrenades.FindOrAdd(GrenadeType) += GrenadeEntry.Count;
			EquipmentComponent->HeldGrenadeEquipment.Add(GrenadeType, GrenadeEntry.Equipment);
			if (InitialGrenadeType == EGrenadeType::None)
			{
				InitialGrenadeType = GrenadeType;
			}
		}
	}

	EquipmentComponent->CurrentGrenadeType = InitialGrenadeType;
	EquipmentComponent->RefreshCurrentGrenadeType();
}

UWeaponCustomization* AAgent::GetResolvedPrimaryWeaponCustomization() const
{
	if (bOverridePrimaryWeapon || !AgentCustomization)
	{
		return PrimaryWeapon;
	}

	return AgentCustomization->GetResolvedDefaultPrimaryWeapon();
}

UWeaponCustomization* AAgent::GetResolvedSecondaryWeaponCustomization() const
{
	if (bOverrideSecondaryWeapon || !AgentCustomization)
	{
		return SecondaryWeapon;
	}

	return AgentCustomization->GetResolvedDefaultSecondaryWeapon();
}

/**
 * @brief Spawns and configures the starting weapons from this agent's setup.
 */
void AAgent::SpawnStartingWeapons()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;

	UWeaponCustomization* ResolvedPrimaryWeapon = GetResolvedPrimaryWeaponCustomization();
	UWeaponCustomization* ResolvedSecondaryWeapon = GetResolvedSecondaryWeaponCustomization();

	if (ResolvedPrimaryWeapon && !EquippedWeapon)
	{
		EquippedWeapon = World->SpawnActor<AWeapon>(AWeapon::StaticClass(), GetActorTransform(), SpawnParameters);
		if (EquippedWeapon)
		{
			const UPawnCustomization* PawnCustomization = GetResolvedPawnCustomization();
			const FName WeaponSocketName = PawnCustomization ? PawnCustomization->RightHandSocketName : TEXT("RightHand");
			EquippedWeapon->OwnerAbilitySystemComponent = AbilitySystemComponent;
			EquippedWeapon->ApplyWeaponCustomization(ResolvedPrimaryWeapon);
			AttachWeaponToSocket(EquippedWeapon, WeaponSocketName);
		}
	}

	if (ResolvedSecondaryWeapon && !StowedWeapon)
	{
		StowedWeapon = World->SpawnActor<AWeapon>(AWeapon::StaticClass(), GetActorTransform(), SpawnParameters);
		if (StowedWeapon)
		{
			StowedWeapon->OwnerAbilitySystemComponent = AbilitySystemComponent;
			StowedWeapon->ApplyWeaponCustomization(ResolvedSecondaryWeapon);
		}
	}
}

/**
 * @brief Attaches a weapon to a mesh socket.
 *
 * @param Weapon Weapon actor to attach.
 * @param SocketName Socket on this agent's mesh.
 */
void AAgent::AttachWeaponToSocket(AWeapon* Weapon, FName SocketName)
{
	if (!Weapon || !Weapon->SkeletalMeshComponent)
	{
		return;
	}

	USkeletalMeshComponent* AgentMesh = GetMesh();
	if (!AgentMesh || !AgentMesh->DoesSocketExist(SocketName))
	{
		return;
	}

	Weapon->AttachToComponent(AgentMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
}

void AAgent::UpdateCurrentAimToTarget(const AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		CurrentAimYaw = 0.0f;
		CurrentAimPitch = 0.0f;
		return;
	}

	const FVector AimOrigin = EquippedWeapon
		? EquippedWeapon->GetMuzzleTransform().GetLocation()
		: GetActorLocation();
	const FTransform MuzzleTransform = EquippedWeapon ? EquippedWeapon->GetMuzzleTransform() : GetActorTransform();
	const FVector ToTarget = TargetActor->GetActorLocation() - AimOrigin;
	if (ToTarget.IsNearlyZero())
	{
		CurrentAimYaw = 0.0f;
		CurrentAimPitch = 0.0f;
		return;
	}

	const FRotator BodyRotation = GetActorRotation();
	const FRotator DesiredAimRotation = ToTarget.Rotation();
	float NewAimYaw = FMath::FindDeltaAngleDegrees(BodyRotation.Yaw, DesiredAimRotation.Yaw);
	float NewAimPitch = FMath::FindDeltaAngleDegrees(BodyRotation.Pitch, DesiredAimRotation.Pitch);

	if (AgentCustomization)
	{
		const FAimingProperties& AimingProperties = AgentCustomization->GetResolvedAimingProperties();
		if (AimingProperties.AimYawLimit > 0.0f)
		{
			NewAimYaw = FMath::Clamp(NewAimYaw, -AimingProperties.AimYawLimit, AimingProperties.AimYawLimit);
		}

		if (AimingProperties.AimPitchLimit > 0.0f)
		{
			NewAimPitch = FMath::Clamp(NewAimPitch, -AimingProperties.AimPitchLimit, AimingProperties.AimPitchLimit);
		}
	}

	CurrentAimYaw = NewAimYaw;
	CurrentAimPitch = NewAimPitch;
}

/**
 * @brief Per-frame update hook for future agent runtime behavior.
 *
 * @param DeltaTime Seconds elapsed since the previous frame.
 */
void AAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetController());
	UpdateCurrentAimToTarget(AgentAIController ? AgentAIController->GetCurrentEnemyTarget() : nullptr);
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

