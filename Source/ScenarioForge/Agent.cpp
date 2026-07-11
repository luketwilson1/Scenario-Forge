// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Agent.cpp
 * @brief Implements the simulated agent character.
 */

#include "Agent.h"

#include "AbilitySystemComponent.h"
#include "ActionDefinition.h"
#include "AgentAIController.h"
#include "AgentCustomization.h"
#include "GameplayAbilitySpec.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "DecisionComponent.h"
#include "EquipmentComponent.h"
#include "EquipmentCustomization.h"
#include "GameFramework/FloatingPawnMovement.h"
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

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(42.0f, 96.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->SetCanEverAffectNavigation(false);
	SetRootComponent(CapsuleComponent);

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CapsuleComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCanEverAffectNavigation(false);

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = CapsuleComponent;
	MovementComponent->MaxSpeed = 600.0f;
	MovementComponent->Acceleration = 2048.0f;
	MovementComponent->Deceleration = 2048.0f;
	MovementComponent->TurningBoost = 8.0f;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	AgentAttributeSet = CreateDefaultSubobject<UAgentAttributeSet>(TEXT("AgentAttributeSet"));
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));

	// TODO: Temporary mesh import alignment fix. Move this into the mesh import/Blueprint setup.
	MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -CapsuleComponent->GetScaledCapsuleHalfHeight()));
	MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

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

UCapsuleComponent* AAgent::GetCapsuleComponent() const
{
	return CapsuleComponent;
}

USkeletalMeshComponent* AAgent::GetMesh() const
{
	return MeshComponent;
}

UPawnMovementComponent* AAgent::GetMovementComponent() const
{
	return MovementComponent;
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
 * @brief Turns this agent so its equipped weapon aims toward a target actor.
 *
 * @param TargetActor Actor this agent should aim at.
 */
void AAgent::AimAtActor(const AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	const FTransform AimTransform = EquippedWeapon ? EquippedWeapon->GetMuzzleTransform() : GetActorTransform();
	const FVector ToTarget = TargetActor->GetActorLocation() - AimTransform.GetLocation();
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const float CurrentMuzzleYaw = AimTransform.GetRotation().GetForwardVector().Rotation().Yaw;
	const float DesiredMuzzleYaw = ToTarget.Rotation().Yaw;
	const float YawDelta = FMath::FindDeltaAngleDegrees(CurrentMuzzleYaw, DesiredMuzzleYaw);
	SetActorRotation(FRotator(CurrentRotation.Pitch, CurrentRotation.Yaw + YawDelta, CurrentRotation.Roll));
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
	GrantAgentAbilities();
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
 * @brief Grants gameplay abilities from the assigned agent sheet.
 */
void AAgent::GrantAgentAbilities()
{
	if (!HasAuthority() || !AgentCustomization || !AbilitySystemComponent)
	{
		return;
	}

	TArray<TSubclassOf<UGameplayAbility>> AbilityClassesToGrant;
	for (const TObjectPtr<UActionDefinition>& ActionDefinition : AgentCustomization->GetResolvedActions())
	{
		if (!ActionDefinition)
		{
			continue;
		}

		for (const TSubclassOf<UGameplayAbility>& AbilityClass : ActionDefinition->RequiredAbilities)
		{
			if (AbilityClass)
			{
				AbilityClassesToGrant.AddUnique(AbilityClass);
			}
		}
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : AbilityClassesToGrant)
	{
		if (!AbilityClass)
		{
			continue;
		}

		bool bAlreadyGranted = false;
		for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
		{
			if (AbilitySpec.Ability && AbilitySpec.Ability->GetClass() == AbilityClass)
			{
				bAlreadyGranted = true;
				break;
			}
		}

		if (!bAlreadyGranted)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
		}
	}
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

	if (GEngine)
	{
		const float Health = AgentAttributeSet->GetHealth();
		const float MaxHealth = AgentAttributeSet->GetMaxHealth();
		const float HealthPercent = MaxHealth > 0.0f
			? FMath::Clamp((Health / MaxHealth) * 100.0f, 0.0f, 100.0f)
			: 0.0f;
		const float CoverVitalityThreshold = AgentCustomization
			? FMath::Clamp(AgentCustomization->GetResolvedCoverProperties().CoverVitalityThreshold, 0.0f, 100.0f)
			: 0.0f;
		GEngine->AddOnScreenDebugMessage(
			INDEX_NONE,
			4.0f,
			FColor::Yellow,
			FString::Printf(
				TEXT("%s took %.0f damage. Health %.0f/%.0f (%.0f%%), CoverVitalityThreshold %.0f%%"),
				*GetName(),
				DamageAmount,
				Health,
				MaxHealth,
				HealthPercent,
				CoverVitalityThreshold));
	}

	if (AgentAttributeSet->GetHealth() <= 0.0f)
	{
		HandleDeath();
	}
	else if (AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetController()))
	{
		AgentAIController->RefreshTacticalMovementMode();
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
		if (UDecisionComponent* DecisionComponent = AgentAIController->GetDecisionComponent())
		{
			DecisionComponent->ShutdownDecisionMaking(TAG_State_Dead.GetTag());
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
			EquippedWeapon->OwnerAbilitySystemComponent = AbilitySystemComponent;
			EquippedWeapon->ApplyWeaponCustomization(ResolvedPrimaryWeapon);
			AttachWeaponToSocket(EquippedWeapon, TEXT("RightHand"));
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

