// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentAIController.cpp
 * @brief Implements AI perception setup and decision-state synchronization.
 */

#include "AgentAIController.h"

#include "AbilitySystemComponent.h"
#include "Agent.h"
#include "AgentCustomization.h"
#include "DecisionComponent.h"
#include "EquipmentComponent.h"
#include "GameplayAbilitySystem/AttributeSets/AgentAttributeSet.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "ScenarioForgeGameplayTags.h"
#include "Squad.h"
#include "TacticalPositioningComponent.h"
#include "TimerManager.h"
#include "EngineUtils.h"

namespace
{
	void SetDecisionStateTag(UDecisionComponent* DecisionComponent, const FGameplayTag& Tag, bool bShouldHaveTag)
	{
		if (!DecisionComponent)
		{
			return;
		}

		if (bShouldHaveTag)
		{
			DecisionComponent->AddCurrentState(Tag);
		}
		else
		{
			DecisionComponent->RemoveCurrentState(Tag);
		}
	}
}

/**
 * @brief Creates the decision and perception components used by agent AI.
 */
AAgentAIController::AAgentAIController()
{
	DecisionComponent = CreateDefaultSubobject<UDecisionComponent>(TEXT("DecisionComponent"));
	TacticalPositioningComponent = CreateDefaultSubobject<UTacticalPositioningComponent>(TEXT("TacticalPositioningComponent"));

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->ConfigureSense(*HearingConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AAgentAIController::HandleTargetPerceptionUpdated);
}

/**
 * @brief Gets the decision component owned by this controller.
 *
 * @return Decision component used for GOAP planning.
 */
UDecisionComponent* AAgentAIController::GetDecisionComponent() const
{
	return DecisionComponent;
}

/**
 * @brief Gets the tactical positioning component owned by this controller.
 *
 * @return Tactical positioning component used for movement intent and destination selection.
 */
UTacticalPositioningComponent* AAgentAIController::GetTacticalPositioningComponent() const
{
	return TacticalPositioningComponent;
}

/**
 * @brief Gets the first valid visible enemy as a temporary combat target.
 *
 * @return First valid seen enemy, or nullptr when none are visible.
 */
AActor* AAgentAIController::GetCurrentEnemyTarget() const
{
	// TODO: Replace this first-valid target fallback with evaluator-based target selection.
	// Evaluators should score visible enemies by criteria such as distance, threat, health, cover, and role.
	for (AActor* SeenEnemy : SeenEnemies)
	{
		if (IsValid(SeenEnemy))
		{
			return SeenEnemy;
		}
	}

	return nullptr;
}

bool AAgentAIController::IsSeeingEnemyActor(const AActor* EnemyActor) const
{
	if (!IsValid(EnemyActor))
	{
		return false;
	}

	return SeenEnemies.ContainsByPredicate([EnemyActor](const TObjectPtr<AActor>& SeenEnemy)
	{
		return SeenEnemy.Get() == EnemyActor;
	});
}

bool AAgentAIController::IsRememberingEnemyActor(const AActor* EnemyActor) const
{
	if (!IsValid(EnemyActor))
	{
		return false;
	}

	return RememberedEnemies.ContainsByPredicate([EnemyActor](const TObjectPtr<AActor>& RememberedEnemy)
	{
		return RememberedEnemy.Get() == EnemyActor;
	});
}

bool AAgentAIController::GetCurrentGrenadeTargetLocation(FVector& OutTargetLocation) const
{
	if (!bHasCurrentGrenadeTargetLocation)
	{
		return false;
	}

	OutTargetLocation = CurrentGrenadeTargetLocation;
	return true;
}

bool AAgentAIController::GetCurrentGrenadeThrowSolution(FGrenadeThrowSolution& OutSolution) const
{
	if (!bHasCurrentGrenadeTargetLocation)
	{
		return false;
	}

	OutSolution = CurrentGrenadeThrowSolution;
	return true;
}

/**
 * @brief Copies customization from the possessed agent and configures controller systems.
 *
 * @param InPawn Pawn newly possessed by this controller.
 */
void AAgentAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const AAgent* Agent = Cast<AAgent>(InPawn);
	AgentCustomization = Agent ? Agent->GetAgentCustomization() : nullptr;
	SeenEnemies.Reset();
	RememberedEnemies.Reset();
	bHasSeenEnemy = false;

	ApplyAgentCustomization();
	BindAbilitySystemStateTags(InPawn);
	RefreshSeesEnemyState();
	RefreshCombatState();
	RefreshGrenadeDecisionState();
}

void AAgentAIController::OnUnPossess()
{
	SetCombatState(EAgentCombatState::Idle);
	ASquad* OwningSquad = FindOwningSquad();
	TArray<TObjectPtr<AActor>> PreviouslySeenEnemies = SeenEnemies;
	TArray<TObjectPtr<AActor>> PreviouslyRememberedEnemies = RememberedEnemies;
	SeenEnemies.Reset();
	RememberedEnemies.Reset();
	RefreshGrenadeDecisionState();

	if (OwningSquad)
	{
		for (AActor* PreviouslySeenEnemy : PreviouslySeenEnemies)
		{
			OwningSquad->RemovePerceivedEnemyIfUnseen(PreviouslySeenEnemy);
		}

		for (AActor* PreviouslyRememberedEnemy : PreviouslyRememberedEnemies)
		{
			OwningSquad->RemoveRememberedTargetIfUnremembered(PreviouslyRememberedEnemy);
		}
	}

	Super::OnUnPossess();
}

/**
 * @brief Subscribes to ability-system tags that should affect GOAP planning.
 *
 * @param InPawn Pawn whose ability system should be observed.
 */
void AAgentAIController::BindAbilitySystemStateTags(APawn* InPawn)
{
	AAgent* Agent = Cast<AAgent>(InPawn);
	UAbilitySystemComponent* AbilitySystemComponent = Agent ? Agent->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->RegisterGameplayTagEvent(
		TAG_State_Weapon_BurstSeparation.GetTag(),
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AAgentAIController::HandleBurstSeparationTagChanged);

	AbilitySystemComponent->RegisterGameplayTagEvent(
		TAG_Cooldown_AI_ThrowGrenade.GetTag(),
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AAgentAIController::HandleGrenadeCooldownTagChanged);
}

/**
 * @brief Determines whether an actor is an agent on a different faction.
 *
 * @param Actor Actor to test.
 * @return True when the actor is an opposing-faction agent.
 */
bool AAgentAIController::IsEnemyActor(const AActor* Actor) const
{
	const AAgent* OtherAgent = Cast<AAgent>(Actor);
	const UAgentCustomization* OtherCustomization = OtherAgent ? OtherAgent->GetAgentCustomization() : nullptr;

	return AgentCustomization
		&& OtherCustomization
		&& AgentCustomization->GetResolvedFaction() != OtherCustomization->GetResolvedFaction();
}

bool AAgentAIController::IsEnemyWithinRememberRadius(const AActor* EnemyActor) const
{
	const AActor* OwnerPawn = GetPawn();
	if (!IsValid(OwnerPawn) || !IsValid(EnemyActor) || !SightConfig)
	{
		return false;
	}

	return FVector::DistSquared(OwnerPawn->GetActorLocation(), EnemyActor->GetActorLocation()) <= FMath::Square(SightConfig->SightRadius);
}

/**
 * @brief Rebuilds the State.SeesEnemy decision tag from the current visible enemies.
 */
void AAgentAIController::RefreshSeesEnemyState()
{
	SeenEnemies.RemoveAll([](const TObjectPtr<AActor>& SeenEnemy)
	{
		return !IsValid(SeenEnemy);
	});
	RememberedEnemies.RemoveAll([](const TObjectPtr<AActor>& RememberedEnemy)
	{
		return !IsValid(RememberedEnemy);
	});

	if (!DecisionComponent)
	{
		return;
	}

	if (!SeenEnemies.IsEmpty())
	{
		bHasSeenEnemy = true;
	}

	if (SeenEnemies.IsEmpty())
	{
		DecisionComponent->RemoveCurrentState(TAG_State_SeesEnemy.GetTag());
		RefreshTacticalMovementMode();
		RestoreNonCombatRotationSettings();
	}
	else
	{
		DecisionComponent->AddCurrentState(TAG_State_SeesEnemy.GetTag());
		AActor* CurrentEnemyTarget = GetCurrentEnemyTarget();
		RefreshTacticalMovementMode();
		ApplyCombatRotationSettings(CurrentEnemyTarget);
	}
}

void AAgentAIController::RefreshCombatState()
{
	SeenEnemies.RemoveAll([](const TObjectPtr<AActor>& SeenEnemy)
	{
		return !IsValid(SeenEnemy);
	});
	RememberedEnemies.RemoveAll([](const TObjectPtr<AActor>& RememberedEnemy)
	{
		return !IsValid(RememberedEnemy);
	});

	bool bSquadCurrentlySeesEnemy = false;
	bool bSquadHasKnownEnemy = false;
	if (const ASquad* OwningSquad = FindOwningSquad())
	{
		for (const FKnownEnemyTarget& KnownEnemyTarget : OwningSquad->KnownEnemyTargets)
		{
			if (!IsValid(KnownEnemyTarget.Actor))
			{
				continue;
			}

			bSquadCurrentlySeesEnemy |= KnownEnemyTarget.bCurrentlySeen;
			bSquadHasKnownEnemy |= KnownEnemyTarget.bCurrentlySeen || KnownEnemyTarget.bRemembered;
		}
	}

	if (!SeenEnemies.IsEmpty() || bSquadCurrentlySeesEnemy)
	{
		SetCombatState(EAgentCombatState::Engage);
		return;
	}

	if (!RememberedEnemies.IsEmpty() || bSquadHasKnownEnemy || bHasSeenEnemy)
	{
		SetCombatState(EAgentCombatState::Alert);
		return;
	}

	SetCombatState(EAgentCombatState::Idle);
}

void AAgentAIController::SetCombatState(EAgentCombatState NewCombatState)
{
	if (CombatState == NewCombatState)
	{
		if (CombatState == EAgentCombatState::Engage)
		{
			StartGrenadeEvalTimer();
		}
		RefreshCombatStateTags();
		return;
	}

	CombatState = NewCombatState;
	RefreshCombatStateTags();

	if (CombatState == EAgentCombatState::Engage)
	{
		StartGrenadeEvalTimer();
		RefreshGrenadeDecisionState();
	}
	else
	{
		StopGrenadeEvalTimer();
		RefreshGrenadeDecisionState();
	}
}

void AAgentAIController::RefreshCombatStateTags()
{
	SetDecisionStateTag(DecisionComponent, TAG_State_Combat_Idle.GetTag(), CombatState == EAgentCombatState::Idle);
	SetDecisionStateTag(DecisionComponent, TAG_State_Combat_Alert.GetTag(), CombatState == EAgentCombatState::Alert);
	SetDecisionStateTag(DecisionComponent, TAG_State_Combat_Engage.GetTag(), CombatState == EAgentCombatState::Engage);
}

void AAgentAIController::StartGrenadeEvalTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float EvalInterval = GetGrenadeEvalInterval();
	World->GetTimerManager().SetTimer(
		GrenadeEvalTimerHandle,
		this,
		&AAgentAIController::RefreshGrenadeDecisionState,
		EvalInterval,
		true);
}

void AAgentAIController::StopGrenadeEvalTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GrenadeEvalTimerHandle);
	}
}

float AAgentAIController::GetGrenadeEvalInterval() const
{
	constexpr float FallbackGrenadeEvalInterval = 0.5f;
	const FGrenadeProperties* GrenadeProperties = GetCurrentGrenadeProperties();
	const float ConfiguredInterval = GrenadeProperties ? GrenadeProperties->GrenadeEvalInterval : 0.0f;
	return ConfiguredInterval > 0.0f ? ConfiguredInterval : FallbackGrenadeEvalInterval;
}

const FGrenadeProperties* AAgentAIController::GetCurrentGrenadeProperties() const
{
	if (!AgentCustomization)
	{
		return nullptr;
	}

	const AAgent* Agent = Cast<AAgent>(GetPawn());
	const UEquipmentComponent* EquipmentComponent = Agent ? Agent->GetEquipmentComponent() : nullptr;
	EGrenadeType GrenadeType = EquipmentComponent ? EquipmentComponent->GetCurrentGrenadeType() : EGrenadeType::None;
	if (GrenadeType == EGrenadeType::None)
	{
		GrenadeType = AgentCustomization->GetResolvedGeneralProperties().DefaultGrenade;
	}

	return AgentCustomization->FindResolvedGrenadeProperties(GrenadeType);
}

void AAgentAIController::RefreshGrenadeDecisionState()
{
	bHasCurrentGrenadeTargetLocation = false;
	CurrentGrenadeTargetLocation = FVector::ZeroVector;
	CurrentGrenadeThrowSolution = FGrenadeThrowSolution();

	if (CombatState != EAgentCombatState::Engage)
	{
		SetDecisionStateTag(DecisionComponent, TAG_State_Grenade_CanThrow.GetTag(), false);
		return;
	}

	APawn* ControlledPawn = GetPawn();
	AAgent* Agent = Cast<AAgent>(ControlledPawn);
	UEquipmentComponent* EquipmentComponent = Agent ? Agent->GetEquipmentComponent() : nullptr;
	const bool bHasUsableGrenade = EquipmentComponent && EquipmentComponent->RefreshCurrentGrenadeType() && EquipmentComponent->HasAnyGrenade();
	const FGrenadeProperties* GrenadeProperties = bHasUsableGrenade ? GetCurrentGrenadeProperties() : nullptr;
	if (!GrenadeProperties)
	{
		SetDecisionStateTag(DecisionComponent, TAG_State_Grenade_CanThrow.GetTag(), false);
		return;
	}

	const int32 MinimumEnemyCount = FMath::Max(0, GrenadeProperties->MinimumEnemyCount);
	const float MinimumRange = FMath::Max(0.0f, GrenadeProperties->GrenadeMinimumRange);
	const float MaximumRange = FMath::Max(0.0f, GrenadeProperties->GrenadeMaximumRange);

	FVector ClusterCenter = FVector::ZeroVector;
	int32 ClusterEnemyCount = 0;
	const bool bEnemiesClustered = MinimumEnemyCount > 0
		&& FindBestGrenadeClusterCenter(ClusterCenter, ClusterEnemyCount)
		&& ClusterEnemyCount >= MinimumEnemyCount;

	const bool bHasValidRange = MaximumRange > 0.0f && MaximumRange >= MinimumRange;
	bool bTargetInRange = false;
	if (bEnemiesClustered && ControlledPawn && bHasValidRange)
	{
		const float DistanceSquared = FVector::DistSquared(ControlledPawn->GetActorLocation(), ClusterCenter);
		bTargetInRange = DistanceSquared >= FMath::Square(MinimumRange)
			&& DistanceSquared <= FMath::Square(MaximumRange);
	}
	FGrenadeThrowSolution ThrowSolution;
	const bool bCanReachTarget = bTargetInRange && CanReachGrenadeTarget(ClusterCenter, *GrenadeProperties, ThrowSolution);
	const bool bCollateralClear = bCanReachTarget && !HasFriendlyInGrenadeCollateralRadius(ClusterCenter, *GrenadeProperties);

	const UAbilitySystemComponent* AbilitySystemComponent = Agent ? Agent->GetAbilitySystemComponent() : nullptr;
	const bool bCooldownReady = !AbilitySystemComponent || !AbilitySystemComponent->HasMatchingGameplayTag(TAG_Cooldown_AI_ThrowGrenade.GetTag());
	const bool bCanThrowGrenade = bHasUsableGrenade && bEnemiesClustered && bTargetInRange && bCanReachTarget && bCollateralClear && bCooldownReady;

	if (bCanThrowGrenade)
	{
		CurrentGrenadeTargetLocation = ClusterCenter;
		CurrentGrenadeThrowSolution = ThrowSolution;
		bHasCurrentGrenadeTargetLocation = true;
	}

	SetDecisionStateTag(DecisionComponent, TAG_State_Grenade_CanThrow.GetTag(), bCanThrowGrenade);
}

bool AAgentAIController::FindBestGrenadeClusterCenter(FVector& OutClusterCenter, int32& OutClusterEnemyCount) const
{
	OutClusterCenter = FVector::ZeroVector;
	OutClusterEnemyCount = 0;

	if (!AgentCustomization)
	{
		return false;
	}

	const FGrenadeProperties* GrenadeProperties = GetCurrentGrenadeProperties();
	if (!GrenadeProperties)
	{
		return false;
	}

	const float EnemyRadius = FMath::Max(0.0f, GrenadeProperties->EnemyRadius);
	if (EnemyRadius <= 0.0f)
	{
		return false;
	}

	TArray<FVector> KnownEnemyLocations;
	if (const ASquad* OwningSquad = FindOwningSquad())
	{
		for (const FKnownEnemyTarget& KnownEnemyTarget : OwningSquad->KnownEnemyTargets)
		{
			if (IsValid(KnownEnemyTarget.Actor) && (KnownEnemyTarget.bCurrentlySeen || KnownEnemyTarget.bRemembered))
			{
				KnownEnemyLocations.Add(KnownEnemyTarget.LastKnownLocation);
			}
		}
	}
	else
	{
		for (AActor* SeenEnemy : SeenEnemies)
		{
			if (IsValid(SeenEnemy))
			{
				KnownEnemyLocations.AddUnique(SeenEnemy->GetActorLocation());
			}
		}

		for (AActor* RememberedEnemy : RememberedEnemies)
		{
			if (IsValid(RememberedEnemy))
			{
				KnownEnemyLocations.AddUnique(RememberedEnemy->GetActorLocation());
			}
		}
	}

	const float EnemyRadiusSquared = FMath::Square(EnemyRadius);
	for (const FVector& CandidateLocation : KnownEnemyLocations)
	{
		FVector ClusterLocationSum = FVector::ZeroVector;
		int32 ClusterCount = 0;
		for (const FVector& KnownEnemyLocation : KnownEnemyLocations)
		{
			if (FVector::DistSquared(CandidateLocation, KnownEnemyLocation) <= EnemyRadiusSquared)
			{
				ClusterLocationSum += KnownEnemyLocation;
				++ClusterCount;
			}
		}

		if (ClusterCount > OutClusterEnemyCount)
		{
			OutClusterEnemyCount = ClusterCount;
			OutClusterCenter = ClusterLocationSum / static_cast<float>(ClusterCount);
		}
	}

	return OutClusterEnemyCount > 0;
}

bool AAgentAIController::CanReachGrenadeTarget(const FVector& TargetLocation, const FGrenadeProperties& GrenadeProperties, FGrenadeThrowSolution& OutSolution) const
{
	const AAgent* ControlledAgent = Cast<AAgent>(GetPawn());
	if (!ControlledAgent)
	{
		return false;
	}

	return UGrenadeThrowFunctionLibrary::BuildThrowSolutionForAgent(ControlledAgent, TargetLocation, GrenadeProperties, OutSolution)
		&& OutSolution.bVelocityInRange
		&& !OutSolution.bTrajectoryBlocked;
}

bool AAgentAIController::HasFriendlyInGrenadeCollateralRadius(const FVector& TargetLocation, const FGrenadeProperties& GrenadeProperties) const
{
	const float CollateralDamageRadius = FMath::Max(0.0f, GrenadeProperties.CollateralDamageRadius);
	if (CollateralDamageRadius <= 0.0f)
	{
		return false;
	}

	const AAgent* ControlledAgent = Cast<AAgent>(GetPawn());
	const ASquad* OwningSquad = FindOwningSquad();
	if (!ControlledAgent || !OwningSquad)
	{
		return false;
	}

	const float CollateralDamageRadiusSquared = FMath::Square(CollateralDamageRadius);
	for (const AAgent* FriendlyAgent : OwningSquad->Agents)
	{
		if (!IsValid(FriendlyAgent) || FriendlyAgent == ControlledAgent)
		{
			continue;
		}

		if (FVector::DistSquared(FriendlyAgent->GetActorLocation(), TargetLocation) <= CollateralDamageRadiusSquared)
		{
			return true;
		}
	}

	return false;
}

ASquad* AAgentAIController::FindOwningSquad() const
{
	const AAgent* Agent = Cast<AAgent>(GetPawn());
	const UWorld* World = GetWorld();
	if (!Agent || !World)
	{
		return nullptr;
	}

	for (TActorIterator<ASquad> It(World); It; ++It)
	{
		ASquad* Squad = *It;
		if (IsValid(Squad) && Squad->Agents.Contains(Agent))
		{
			return Squad;
		}
	}

	return nullptr;
}

/**
 * @brief Re-evaluates tactical movement mode from perception and cover-condition state.
 */
void AAgentAIController::RefreshTacticalMovementMode()
{
	if (!TacticalPositioningComponent)
	{
		return;
	}

	if (ShouldUseCoverMovement())
	{
		TacticalPositioningComponent->SetTargetActor(GetCurrentEnemyTarget());
		if (TacticalPositioningComponent->GetMovementMode() != ETacticalMovementMode::Uncover)
		{
			TacticalPositioningComponent->SetMovementMode(ETacticalMovementMode::Cover);
		}
		return;
	}

	if (!SeenEnemies.IsEmpty())
	{
		TacticalPositioningComponent->SetTargetActor(GetCurrentEnemyTarget());
		TacticalPositioningComponent->SetMovementMode(ETacticalMovementMode::Combat);
		return;
	}

	if (bHasSeenEnemy)
	{
		TacticalPositioningComponent->ClearTargetActor();
		TacticalPositioningComponent->SetMovementMode(ETacticalMovementMode::Search);
		return;
	}

	if (TacticalPositioningComponent->GetMovementMode() == ETacticalMovementMode::None)
	{
		TacticalPositioningComponent->ClearTargetActor();
		TacticalPositioningComponent->SetMovementMode(ETacticalMovementMode::Idle);
	}
}

/**
 * @brief Returns whether cover movement should be preferred over normal combat movement.
 */
bool AAgentAIController::ShouldUseCoverMovement() const
{
	const float CoverVitalityThreshold = AgentCustomization
		? FMath::Clamp(AgentCustomization->GetResolvedCoverProperties().CoverVitalityThreshold, 0.0f, 100.0f)
		: 0.0f;
	if (CoverVitalityThreshold <= 0.0f)
	{
		return false;
	}

	const AAgent* Agent = Cast<AAgent>(GetPawn());
	const UAbilitySystemComponent* AbilitySystemComponent = Agent ? Agent->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		return false;
	}

	const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(UAgentAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= 0.0f)
	{
		return false;
	}

	const float Health = AbilitySystemComponent->GetNumericAttribute(UAgentAttributeSet::GetHealthAttribute());
	const float HealthPercent = FMath::Clamp((Health / MaxHealth) * 100.0f, 0.0f, 100.0f);
	return HealthPercent <= CoverVitalityThreshold;
}

/**
 * @brief Makes combat target focus own yaw instead of movement direction.
 *
 * @param FocusTarget Enemy target to face while moving and shooting.
 */
void AAgentAIController::ApplyCombatRotationSettings(AActor* FocusTarget)
{
	AAgent* Agent = Cast<AAgent>(GetPawn());
	if (!Agent)
	{
		return;
	}

	if (!bHasSavedRotationSettings)
	{
		bSavedUseControllerRotationYaw = Agent->bUseControllerRotationYaw;
		bHasSavedRotationSettings = true;
	}

	Agent->bUseControllerRotationYaw = true;

	if (FocusTarget)
	{
		SetFocus(FocusTarget, EAIFocusPriority::Gameplay);
	}
}

/**
 * @brief Restores normal movement-facing rotation after combat movement ends.
 */
void AAgentAIController::RestoreNonCombatRotationSettings()
{
	AAgent* Agent = Cast<AAgent>(GetPawn());
	if (Agent && bHasSavedRotationSettings)
	{
		Agent->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
	}

	ClearFocus(EAIFocusPriority::Gameplay);
	bHasSavedRotationSettings = false;
}

/**
 * @brief Responds to sight perception changes and keeps the visible enemy list current.
 *
 * @param Actor Actor whose perception state changed.
 * @param Stimulus Perception stimulus reported by Unreal.
 */
void AAgentAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed() && IsEnemyActor(Actor))
	{
		const int32 RemovedRememberedCount = RememberedEnemies.RemoveAll([Actor](const TObjectPtr<AActor>& RememberedEnemy)
		{
			return RememberedEnemy.Get() == Actor;
		});
		const bool bAlreadySeen = SeenEnemies.ContainsByPredicate([Actor](const TObjectPtr<AActor>& SeenEnemy)
		{
			return SeenEnemy.Get() == Actor;
		});

		if (RemovedRememberedCount > 0)
		{
			if (ASquad* OwningSquad = FindOwningSquad())
			{
				OwningSquad->RemoveRememberedTargetIfUnremembered(Actor);
			}
		}

		if (!bAlreadySeen)
		{
			SeenEnemies.Add(Actor);
			if (ASquad* OwningSquad = FindOwningSquad())
			{
				OwningSquad->AddPerceivedEnemy(Actor);
			}
		}
	}
	else
	{
		const bool bShouldRememberEnemy = IsEnemyActor(Actor) && IsEnemyWithinRememberRadius(Actor);
		const int32 RemovedCount = SeenEnemies.RemoveAll([Actor](const TObjectPtr<AActor>& SeenEnemy)
		{
			return SeenEnemy.Get() == Actor;
		});

		if (RemovedCount > 0)
		{
			if (ASquad* OwningSquad = FindOwningSquad())
			{
				OwningSquad->RemovePerceivedEnemyIfUnseen(Actor);
			}
		}

		if (bShouldRememberEnemy)
		{
			RememberedEnemies.AddUnique(Actor);
			if (ASquad* OwningSquad = FindOwningSquad())
			{
				OwningSquad->AddRememberedTarget(Actor);
			}
		}
		else
		{
			const int32 RemovedRememberedCount = RememberedEnemies.RemoveAll([Actor](const TObjectPtr<AActor>& RememberedEnemy)
			{
				return RememberedEnemy.Get() == Actor;
			});
			if (RemovedRememberedCount > 0)
			{
				if (ASquad* OwningSquad = FindOwningSquad())
				{
					OwningSquad->RemoveRememberedTargetIfUnremembered(Actor);
				}
			}
		}
	}

	RefreshSeesEnemyState();
	RefreshCombatState();
	RefreshGrenadeDecisionState();
}

/**
 * @brief Mirrors burst separation gameplay effect state into the decision component.
 *
 * @param Tag Tag whose count changed.
 * @param NewCount New active count for the tag.
 */
void AAgentAIController::HandleBurstSeparationTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!DecisionComponent || !Tag.MatchesTagExact(TAG_State_Weapon_BurstSeparation.GetTag()))
	{
		return;
	}

	if (NewCount > 0)
	{
		DecisionComponent->AddCurrentState(TAG_State_Weapon_BurstSeparation.GetTag());
	}
	else
	{
		DecisionComponent->RemoveCurrentState(TAG_State_Weapon_BurstSeparation.GetTag());
	}
}

void AAgentAIController::HandleGrenadeCooldownTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	(void)NewCount;

	if (!Tag.MatchesTagExact(TAG_Cooldown_AI_ThrowGrenade.GetTag()))
	{
		return;
	}

	RefreshGrenadeDecisionState();
}

/**
 * @brief Applies agent sheet values to decision actions and AI perception ranges.
 */
void AAgentAIController::ApplyAgentCustomization()
{
	if (!AgentCustomization)
	{
		return;
	}

	if (DecisionComponent)
	{
		/** Push data-asset actions and goals into the planner when this controller possesses an agent. */
		DecisionComponent->SetActions(AgentCustomization->GetResolvedActions());
		DecisionComponent->SetGoalStates(AgentCustomization->GetResolvedStartingGoalTags());
	}

	const FPerception& ResolvedPerception = AgentCustomization->GetResolvedPerception();
	if (SightConfig)
	{
		SightConfig->SightRadius = ResolvedPerception.SightRadius;
		SightConfig->LoseSightRadius = ResolvedPerception.LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = ResolvedPerception.PeripheralVisionAngleDegrees;
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = ResolvedPerception.HearingRange;
	}

	if (AIPerceptionComponent && SightConfig && HearingConfig)
	{
		AIPerceptionComponent->ConfigureSense(*SightConfig);
		AIPerceptionComponent->ConfigureSense(*HearingConfig);
		AIPerceptionComponent->RequestStimuliListenerUpdate();
	}
}
