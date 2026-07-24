// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AI/Agent.cpp
 * @brief Implements the simulated agent character.
 */

#include "Agent.h"

#include "AbilitySystemComponent.h"
#include "AgentAIController.h"
#include "AgentSheet.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Planner.h"
#include "EquipmentComponent.h"
#include "EquipmentSheet.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySystem/AttributeSets/AgentAttributeSet.h"
#include "GameplayAbilitySystem/Abilities/GA_ThrowGrenade.h"
#include "GameplayAbilitySystem/Abilities/GA_Melee.h"
#include "GameplayAbilitySystem/GameplayEffects/GE_Damage.h"
#include "GameplayAbilitySpec.h"
#include "PawnSheet.h"
#include "Net/UnrealNetwork.h"
#include "Navigation/PathFollowingComponent.h"
#include "Projectile.h"
#include "ScenarioForgeGameplayTags.h"
#include "TimerManager.h"
#include "Weapon.h"
#include "WeaponSheet.h"

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
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	AgentAttributeSet = CreateDefaultSubobject<UAgentAttributeSet>(TEXT("AgentAttributeSet"));
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));

	FiredUponDetectionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("FiredUponDetectionComponent"));
	FiredUponDetectionComponent->SetupAttachment(GetCapsuleComponent());
	FiredUponDetectionComponent->InitSphereRadius(500.0f);
	FiredUponDetectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FiredUponDetectionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	FiredUponDetectionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	FiredUponDetectionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	FiredUponDetectionComponent->SetGenerateOverlapEvents(true);
	FiredUponDetectionComponent->SetCanEverAffectNavigation(false);
	FiredUponDetectionComponent->OnComponentBeginOverlap.AddDynamic(this, &AAgent::HandleFiredUponBeginOverlap);

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
 * @return Assigned agent sheet asset.
 */
UAgentSheet* AAgent::GetAgentSheet() const
{
	return AgentSheet;
}

EFaction AAgent::GetResolvedFaction() const
{
	return bOverrideFaction || !AgentSheet
		? Faction
		: AgentSheet->GetResolvedFaction();
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

const UPawnSheet* AAgent::GetResolvedPawnSheet() const
{
	return AgentSheet ? AgentSheet->GetResolvedPawnSheet() : nullptr;
}

FTransform AAgent::GetGrenadeReleaseTransform() const
{
	const UPawnSheet* PawnSheet = GetResolvedPawnSheet();
	const FName SocketName = PawnSheet ? PawnSheet->GrenadeReleaseSocketName : NAME_None;
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
 * @brief Applies construction-time sheet after editable properties change.
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

	ApplyAgentSheet();
}

/**
 * @brief Initializes runtime GAS state, sheet, and starting equipment.
 */
void AAgent::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	if (HasAuthority())
	{
		/** Every agent receives the base grenade ability; the GOAP action still requires grenade inventory and a valid throw solution. */
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_ThrowGrenade::StaticClass(), 1));
		/** Every agent receives melee; its GOAP action still requires a valid visible target in melee range. */
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_Melee::StaticClass(), 1));
	}
	const float InitialMaxHealth = AgentSheet
		? FMath::Max(0.0f, AgentSheet->GetResolvedVitalityProperties().MaxHealth)
		: 100.0f;
	AgentAttributeSet->InitMaxHealth(InitialMaxHealth);
	AgentAttributeSet->InitHealth(AgentAttributeSet->GetMaxHealth());
	HealthChangedDelegateHandle = AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(UAgentAttributeSet::GetHealthAttribute())
		.AddUObject(this, &AAgent::HandleHealthChanged);

	ApplyAgentSheet();
	InitializeStartingEquipment();
	SpawnStartingWeapons();
	EvaluateDownedState();
}

/**
 * @brief Clears transient nearby-fire state before the agent leaves play.
 *
 * @param EndPlayReason Reason Unreal is ending play for this actor.
 */
void AAgent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent && HealthChangedDelegateHandle.IsValid())
	{
		AbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(UAgentAttributeSet::GetHealthAttribute())
			.Remove(HealthChangedDelegateHandle);
		HealthChangedDelegateHandle.Reset();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FiredUponStateTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
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

void AAgent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	if (ChangeData.NewValue <= 0.0f)
	{
		HandleDeath();
		return;
	}

	EvaluateDownedState();
}

void AAgent::EvaluateDownedState()
{
	if (!HasAuthority() || bIsDead || !AgentSheet || !AgentAttributeSet)
	{
		return;
	}

	const FVitalityProperties& Vitality = AgentSheet->GetResolvedVitalityProperties();
	const bool bShouldBeDowned = Vitality.bCanBeDowned
		&& AgentAttributeSet->GetHealth() <= FMath::Max(0.0f, Vitality.DownedHealthThreshold);
	SetDowned(bShouldBeDowned);
}

void AAgent::SetDowned(const bool bNewDowned)
{
	if (!HasAuthority() || bIsDead || bIsDowned == bNewDowned)
	{
		return;
	}

	bIsDowned = bNewDowned;

	AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetController());
	/** Suspend the planner before cancelling abilities so their callbacks cannot launch another action. */
	if (bIsDowned && AgentAIController)
	{
		AgentAIController->SetAgentDowned(true);
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetUserAbilityActivationInhibited(bIsDowned);
		if (bIsDowned)
		{
			AbilitySystemComponent->AddLooseGameplayTag(TAG_State_Downed.GetTag());
			AbilitySystemComponent->CancelAllAbilities();
		}
		else
		{
			AbilitySystemComponent->RemoveLooseGameplayTag(TAG_State_Downed.GetTag());
		}
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (bIsDowned)
		{
			MovementComponent->DisableMovement();
		}
		else if (MovementComponent->MovementMode == MOVE_None)
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
	}

	if (bIsDowned)
	{
		if (USkeletalMeshComponent* MeshComponent = GetMesh())
		{
			if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
			{
				AnimInstance->StopAllMontages(0.1f);
			}
		}

		UpdateCurrentAimToTarget(FVector::ZeroVector, false, 0.0f);
	}
	else if (AgentAIController)
	{
		/** Resume only after movement and ability activation have been restored. */
		AgentAIController->SetAgentDowned(false);
	}
}

void AAgent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAgent, bIsPeekingFromCover);
	DOREPLIFETIME(AAgent, bIsDowned);
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

void AAgent::AddGrenadeDangerSource(AProjectile* DangerSource)
{
	if (!IsValid(DangerSource))
	{
		return;
	}

	GrenadeDangerSources.Add(DangerSource);
	SetCurrentDangerSourceLocation(DangerSource->GetActorLocation());
}

void AAgent::RemoveGrenadeDangerSource(AProjectile* DangerSource)
{
	GrenadeDangerSources.Remove(DangerSource);
	for (auto It = GrenadeDangerSources.CreateIterator(); It; ++It)
	{
		if (AProjectile* RemainingSource = It->Get())
		{
			SetCurrentDangerSourceLocation(RemainingSource->GetActorLocation());
			return;
		}

		It.RemoveCurrent();
	}

	ClearCurrentDangerSourceLocation();
}

void AAgent::GetGrenadeDangerSources(TArray<AActor*>& OutDangerSources) const
{
	OutDangerSources.Reset();
	for (const TWeakObjectPtr<AProjectile>& WeakSource : GrenadeDangerSources)
	{
		if (AProjectile* Source = WeakSource.Get())
		{
			OutDangerSources.Add(Source);
		}
	}
}

bool AAgent::HasGrenadeDangerSources() const
{
	for (const TWeakObjectPtr<AProjectile>& WeakSource : GrenadeDangerSources)
	{
		if (WeakSource.IsValid())
		{
			return true;
		}
	}

	return false;
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

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AAgentAIController> It(World); It; ++It)
		{
			It->HandleEnemyAgentDeath(this);
		}
	}

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
 * @brief Applies visual sheet data from the assigned agent sheet.
 */
void AAgent::ApplyAgentSheet()
{
	ConfigureFiredUponDetection();

	if (!AgentSheet)
	{
		return;
	}

	const FAppearance& ResolvedAppearance = AgentSheet->GetResolvedAppearance();
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
 * @brief Configures the spherical hostile-bullet query volume from the inherited Agent Sheet values.
 */
void AAgent::ConfigureFiredUponDetection()
{
	if (!FiredUponDetectionComponent)
	{
		return;
	}

	const FFiredUponProperties* Properties = AgentSheet
		? &AgentSheet->GetResolvedFiredUponProperties()
		: nullptr;
	const float DetectionRadius = Properties ? FMath::Max(0.0f, Properties->DetectionRadius) : 0.0f;
	const bool bEnableDetection = DetectionRadius > 0.0f && Properties->StateRefreshDuration > 0.0f;
	FiredUponDetectionComponent->SetSphereRadius(FMath::Max(1.0f, DetectionRadius), true);
	FiredUponDetectionComponent->SetCollisionEnabled(
		bEnableDetection ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

/**
 * @brief Adds or refreshes State.FiredUpon when a hostile non-explosive projectile passes nearby.
 */
void AAgent::HandleFiredUponBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	AProjectile* Projectile = HasAuthority() ? Cast<AProjectile>(OtherActor) : nullptr;
	AAgent* FiringAgent = Projectile ? Cast<AAgent>(Projectile->GetInstigator()) : nullptr;
	if (!Projectile || Projectile->IsGrenadeDangerProjectile() || !FiringAgent || FiringAgent == this || IsDead() || IsDowned())
	{
		return;
	}

	if (GetResolvedFaction() == FiringAgent->GetResolvedFaction())
	{
		return;
	}

	AAgentAIController* AgentController = Cast<AAgentAIController>(GetController());
	UPlanner* Planner = AgentController ? AgentController->GetPlanner() : nullptr;
	if (!Planner)
	{
		return;
	}

	Planner->AddCurrentState(TAG_State_FiredUpon.GetTag());
	const float RefreshDuration = AgentSheet->GetResolvedFiredUponProperties().StateRefreshDuration;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FiredUponStateTimerHandle,
			this,
			&AAgent::ExpireFiredUponState,
			FMath::Max(0.01f, RefreshDuration),
			false);
	}
}

/**
 * @brief Removes the transient fired-upon planner state after nearby fire stops.
 */
void AAgent::ExpireFiredUponState()
{
	AAgentAIController* AgentController = Cast<AAgentAIController>(GetController());
	if (UPlanner* Planner = AgentController ? AgentController->GetPlanner() : nullptr)
	{
		Planner->RemoveCurrentState(TAG_State_FiredUpon.GetTag());
	}
}

/**
 * @brief Applies starting equipment counts from this agent's sheet sheet.
 */
void AAgent::InitializeStartingEquipment()
{
	if (!EquipmentComponent)
	{
		return;
	}

	EquipmentComponent->HeldGrenades.Reset();
	EquipmentComponent->HeldGrenadeEquipment.Reset();
	const TArray<FStartingGrenade>& ResolvedStartingGrenades = bOverrideStartingGrenades || !AgentSheet
		? StartingGrenades
		: AgentSheet->GetResolvedStartingGrenades();
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

UWeaponSheet* AAgent::GetResolvedPrimaryWeaponSheet() const
{
	if (bOverridePrimaryWeapon || !AgentSheet)
	{
		return PrimaryWeapon;
	}

	return AgentSheet->GetResolvedDefaultPrimaryWeapon();
}

UWeaponSheet* AAgent::GetResolvedSecondaryWeaponSheet() const
{
	if (bOverrideSecondaryWeapon || !AgentSheet)
	{
		return SecondaryWeapon;
	}

	return AgentSheet->GetResolvedDefaultSecondaryWeapon();
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

	UWeaponSheet* ResolvedPrimaryWeapon = GetResolvedPrimaryWeaponSheet();
	UWeaponSheet* ResolvedSecondaryWeapon = GetResolvedSecondaryWeaponSheet();

	if (ResolvedPrimaryWeapon && !EquippedWeapon)
	{
		EquippedWeapon = World->SpawnActor<AWeapon>(AWeapon::StaticClass(), GetActorTransform(), SpawnParameters);
		if (EquippedWeapon)
		{
			const UPawnSheet* PawnSheet = GetResolvedPawnSheet();
			const FName WeaponSocketName = PawnSheet ? PawnSheet->RightHandSocketName : TEXT("RightHand");
			EquippedWeapon->OwnerAbilitySystemComponent = AbilitySystemComponent;
			EquippedWeapon->ApplyWeaponSheet(ResolvedPrimaryWeapon);
			AttachWeaponToSocket(EquippedWeapon, WeaponSocketName);
		}
	}

	if (ResolvedSecondaryWeapon && !StowedWeapon)
	{
		StowedWeapon = World->SpawnActor<AWeapon>(AWeapon::StaticClass(), GetActorTransform(), SpawnParameters);
		if (StowedWeapon)
		{
			StowedWeapon->OwnerAbilitySystemComponent = AbilitySystemComponent;
			StowedWeapon->ApplyWeaponSheet(ResolvedSecondaryWeapon);
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

void AAgent::DisableAutomaticMovementFacing()
{
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = false;
		MovementComponent->bUseControllerDesiredRotation = false;
	}
}

void AAgent::UpdateCurrentAimToTarget(
	const FVector& TargetLocation,
	const bool bHasTargetLocation,
	const float DeltaTime)
{
	if (!bHasTargetLocation)
	{
		CurrentAimYaw = 0.0f;
		CurrentAimPitch = 0.0f;
		bBodyTurnInProgress = false;
		return;
	}

	const FVector AimOrigin = EquippedWeapon
		? EquippedWeapon->GetMuzzleTransform().GetLocation()
		: GetActorLocation();
	const FVector ToTarget = TargetLocation - AimOrigin;
	if (ToTarget.IsNearlyZero())
	{
		CurrentAimYaw = 0.0f;
		CurrentAimPitch = 0.0f;
		bBodyTurnInProgress = false;
		return;
	}

	const FRotator DesiredAimRotation = ToTarget.Rotation();
	FRotator BodyRotation = GetActorRotation();
	float NewAimYaw = FMath::FindDeltaAngleDegrees(BodyRotation.Yaw, DesiredAimRotation.Yaw);

	if (AgentSheet)
	{
		const FAimingProperties& AimingProperties = AgentSheet->GetResolvedAimingProperties();
		const float AimYawLimit = FMath::Max(0.0f, AimingProperties.AimYawLimit);
		const float BodyTurnStep = FMath::Max(0.0f, AimingProperties.BodyTurnStepDegrees);
		const float BodyTurnSpeed = FMath::Max(0.0f, AimingProperties.BodyTurnSpeed);
		const AAgentAIController* AgentController = Cast<AAgentAIController>(GetController());
		const UPlanner* AgentPlanner = AgentController ? AgentController->GetPlanner() : nullptr;
		const bool bHasReachedCover = AgentPlanner
			&& AgentPlanner->CurrentStates.HasTagExact(TAG_State_InCover.GetTag());
		const bool bIsFollowingPath = AgentController
			&& AgentController->GetMoveStatus() == EPathFollowingStatus::Moving;
		/**
		 * Preserve the authored cover-facing rotation while tucked into cover, but
		 * allow target tracking while approaching cover or traversing to/from a peek
		 * point. A reservation is acquired before FindCover movement starts, so the
		 * claim alone cannot be used as proof that the agent is already tucked in.
		 */
		const bool bLockBodyToCover = AgentController
			&& AgentController->HasCoverClaim()
			&& bHasReachedCover
			&& !bIsPeekingFromCover
			&& !bIsFollowingPath;
		if (bLockBodyToCover)
		{
			bBodyTurnInProgress = false;
		}

		if (!bLockBodyToCover
			&& !bBodyTurnInProgress
			&& AimYawLimit > 0.0f
			&& FMath::Abs(NewAimYaw) > AimYawLimit
			&& BodyTurnStep > 0.0f
			&& BodyTurnSpeed > 0.0f)
		{
			const float TurnAmount = FMath::Min(BodyTurnStep, FMath::Abs(NewAimYaw));
			BodyTurnTargetYaw = FRotator::NormalizeAxis(BodyRotation.Yaw + FMath::Sign(NewAimYaw) * TurnAmount);
			bBodyTurnInProgress = true;
		}

		if (!bLockBodyToCover && bBodyTurnInProgress)
		{
			const float NewBodyYaw = FMath::FixedTurn(
				BodyRotation.Yaw,
				BodyTurnTargetYaw,
				BodyTurnSpeed * FMath::Max(0.0f, DeltaTime));
			SetActorRotation(FRotator(BodyRotation.Pitch, NewBodyYaw, BodyRotation.Roll));
			BodyRotation = GetActorRotation();
			NewAimYaw = FMath::FindDeltaAngleDegrees(BodyRotation.Yaw, DesiredAimRotation.Yaw);

			if (FMath::Abs(FMath::FindDeltaAngleDegrees(BodyRotation.Yaw, BodyTurnTargetYaw)) <= 0.1f)
			{
				bBodyTurnInProgress = false;
			}
		}

		if (AimingProperties.AimYawLimit > 0.0f)
		{
			NewAimYaw = FMath::Clamp(NewAimYaw, -AimingProperties.AimYawLimit, AimingProperties.AimYawLimit);
		}
	}

	float NewAimPitch = FMath::FindDeltaAngleDegrees(BodyRotation.Pitch, DesiredAimRotation.Pitch);
	if (AgentSheet)
	{
		const float AimPitchLimit = AgentSheet->GetResolvedAimingProperties().AimPitchLimit;
		if (AimPitchLimit > 0.0f)
		{
			NewAimPitch = FMath::Clamp(NewAimPitch, -AimPitchLimit, AimPitchLimit);
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

	if (bIsDead || bIsDowned)
	{
		UpdateCurrentAimToTarget(FVector::ZeroVector, false, DeltaTime);
		return;
	}

	AAgentAIController* AgentAIController = Cast<AAgentAIController>(GetController());
	FVector AimTargetLocation = FVector::ZeroVector;
	const bool bHasAimTarget = AgentAIController
		&& AgentAIController->GetCurrentAimTargetLocation(AimTargetLocation);
	UpdateCurrentAimToTarget(AimTargetLocation, bHasAimTarget, DeltaTime);
	if (AgentAIController)
	{
		AgentAIController->RefreshAttackRangeStates();
	}
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

