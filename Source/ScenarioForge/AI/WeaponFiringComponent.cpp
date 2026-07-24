// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file WeaponFiringComponent.cpp
 * @brief Implements concurrent visible-target bursts and deliberate GOAP weapon requests.
 */

#include "WeaponFiringComponent.h"

#include "Actions/Action.h"
#include "Agent.h"
#include "AgentAIController.h"
#include "AgentSheet.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySystem/GameplayEffects/GE_BurstSeparation.h"
#include "Planner.h"
#include "ScenarioForgeGameplayTags.h"
#include "Weapon.h"
#include "WeaponSheet.h"

UWeaponFiringComponent::UWeaponFiringComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
}

bool UWeaponFiringComponent::BeginDeliberateFire(UPlanner* Planner)
{
	if (!Planner || (DeliberatePlanner.IsValid() && DeliberatePlanner.Get() != Planner))
	{
		return false;
	}

	AActor* Target = nullptr;
	AAgent* Agent = nullptr;
	AWeapon* Weapon = nullptr;
	if (!ResolveFireContext(Target, Agent, Weapon))
	{
		return false;
	}

	/** A deliberate planner action takes ownership from any zero-separation opportunistic burst. */
	StopActiveBurst();
	DeliberatePlanner = Planner;
	const UAgentSheet* Sheet = Agent->GetAgentSheet();
	const float AlignmentTimeout = Sheet
		? FMath::Max(0.0f, Sheet->GetResolvedAimingProperties().FireAlignmentTimeout)
		: 0.0f;
	const UWorld* World = GetWorld();
	DeliberateAlignmentDeadline = (World ? World->GetTimeSeconds() : 0.0) + AlignmentTimeout;
	return true;
}

bool UWeaponFiringComponent::CancelDeliberateFire(const UPlanner* Planner)
{
	if (!Planner || DeliberatePlanner.Get() != Planner)
	{
		return false;
	}

	StopActiveBurst();
	DeliberatePlanner.Reset();
	DeliberateAlignmentDeadline = 0.0;
	return true;
}

void UWeaponFiringComponent::StopAllFiring()
{
	StopActiveBurst();
	DeliberatePlanner.Reset();
	DeliberateAlignmentDeadline = 0.0;
	NextOpportunisticFireTime = 0.0;
}

void UWeaponFiringComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Target = nullptr;
	AAgent* Agent = nullptr;
	AWeapon* Weapon = nullptr;
	/** A separation effect may begin with the burst; it blocks the next burst, not the one already running. */
	const bool bHasFireContext = ResolveFireContext(Target, Agent, Weapon, bBurstInProgress);
	UWorld* World = GetWorld();
	const double CurrentTime = World ? World->GetTimeSeconds() : 0.0;

	if (DeliberatePlanner.IsValid())
	{
		if (!bHasFireContext)
		{
			CompleteDeliberateFire(false);
			return;
		}

		if (bBurstInProgress)
		{
			return;
		}

		const UAgentSheet* Sheet = Agent->GetAgentSheet();
		const float AimDeviation = Sheet
			? FMath::Clamp(Sheet->GetResolvedAimingProperties().AllowableFireAimDeviation, 0.0f, 180.0f)
			: 0.0f;
		if (Weapon->IsMuzzleAlignedWithTarget(Target, AimDeviation))
		{
			if (!StartBurst(Target, Agent, Weapon, true))
			{
				CompleteDeliberateFire(false);
			}
			return;
		}

		if (CurrentTime >= DeliberateAlignmentDeadline)
		{
			CompleteDeliberateFire(false);
		}
		return;
	}

	const bool bShouldFire = bHasFireContext && ShouldFireOpportunistically();
	if (bBurstInProgress)
	{
		if (!bShouldFire)
		{
			StopActiveBurst();
		}
		return;
	}

	if (!bShouldFire || CurrentTime < NextOpportunisticFireTime)
	{
		return;
	}

	const UAgentSheet* Sheet = Agent->GetAgentSheet();
	const float AimDeviation = Sheet
		? FMath::Clamp(Sheet->GetResolvedAimingProperties().AllowableFireAimDeviation, 0.0f, 180.0f)
		: 0.0f;
	if (Weapon->IsMuzzleAlignedWithTarget(Target, AimDeviation))
	{
		StartBurst(Target, Agent, Weapon, false);
	}
}

bool UWeaponFiringComponent::ResolveOwner(AAgentAIController*& OutController, AAgent*& OutAgent) const
{
	OutController = Cast<AAgentAIController>(GetOwner());
	OutAgent = OutController ? Cast<AAgent>(OutController->GetPawn()) : nullptr;
	return OutController && OutAgent;
}

bool UWeaponFiringComponent::ResolveFireContext(
	AActor*& OutTarget,
	AAgent*& OutAgent,
	AWeapon*& OutWeapon,
	const bool bIgnoreBurstSeparation) const
{
	AAgentAIController* Controller = nullptr;
	if (!ResolveOwner(Controller, OutAgent))
	{
		return false;
	}

	UPlanner* Planner = Controller->GetPlanner();
	OutTarget = Controller->GetCurrentEnemyTarget();
	OutWeapon = OutAgent->GetPrimaryWeapon();
	const UAgentSheet* Sheet = OutAgent->GetAgentSheet();
	const UWeaponSheet* WeaponSheet = OutWeapon ? OutWeapon->GetActiveWeaponSheet() : nullptr;
	return Planner
		&& !OutAgent->IsDowned()
		&& IsValid(OutTarget)
		&& OutWeapon
		&& Sheet
		&& WeaponSheet
		&& Planner->CurrentStates.HasTagExact(TAG_State_SeesEnemy.GetTag())
		&& Planner->CurrentStates.HasTagExact(TAG_State_InWeaponRange.GetTag())
		&& !Planner->CurrentStates.HasTagExact(TAG_State_Dead.GetTag())
		&& (bIgnoreBurstSeparation
			|| !Planner->CurrentStates.HasTagExact(TAG_State_Weapon_BurstSeparation.GetTag()));
}

bool UWeaponFiringComponent::ShouldFireOpportunistically() const
{
	const AAgentAIController* Controller = Cast<AAgentAIController>(GetOwner());
	const UPlanner* Planner = Controller ? Controller->GetPlanner() : nullptr;
	const UAction* ActiveAction = Planner ? Planner->GetActiveAction() : nullptr;
	return ActiveAction
		&& ActiveAction->ConcurrentFirePolicy == EConcurrentFirePolicy::VisibleTargets;
}

bool UWeaponFiringComponent::StartBurst(
	AActor* Target,
	AAgent* Agent,
	AWeapon* Weapon,
	const bool bDeliberate)
{
	UAgentSheet* Sheet = Agent ? Agent->GetAgentSheet() : nullptr;
	UWeaponSheet* WeaponSheet = Weapon ? Weapon->GetActiveWeaponSheet() : nullptr;
	const FWeaponProperties* WeaponProperties = Sheet && WeaponSheet
		? Sheet->FindResolvedWeaponProperties(WeaponSheet)
		: nullptr;
	if (!IsValid(Target) || !Agent || !Weapon || !Sheet || !WeaponSheet || !WeaponProperties)
	{
		return false;
	}

	const float BurstDuration = FMath::FRandRange(
		FMath::Min(WeaponProperties->MinimumBurstDuration, WeaponProperties->MaximumBurstDuration),
		FMath::Max(WeaponProperties->MinimumBurstDuration, WeaponProperties->MaximumBurstDuration));
	const float BurstSeparation = FMath::FRandRange(
		FMath::Min(WeaponProperties->MinimumBurstSeparation, WeaponProperties->MaximumBurstSeparation),
		FMath::Max(WeaponProperties->MinimumBurstSeparation, WeaponProperties->MaximumBurstSeparation));
	const float AimDeviation = FMath::Clamp(
		Sheet->GetResolvedAimingProperties().AllowableFireAimDeviation,
		0.0f,
		180.0f);
	const bool bTimedBurst = BurstDuration > 0.0f && WeaponSheet->RateOfFire > 0;

	ActiveBurstWeapon = Weapon;
	bBurstInProgress = bTimedBurst;
	bActiveBurstIsDeliberate = bDeliberate;
	if (bTimedBurst)
	{
		FOnWeaponBurstFinished OnFinished;
		OnFinished.BindUObject(this, &UWeaponFiringComponent::HandleBurstFinished);
		Weapon->FireBurst(Target, AimDeviation, BurstDuration, MoveTemp(OnFinished));
	}
	else
	{
		Weapon->FireBurst(Target, AimDeviation, BurstDuration);
	}

	ApplyBurstSeparation(Agent, BurstSeparation);
	const float MinimumRetryInterval = WeaponSheet->RateOfFire > 0
		? 1.0f / static_cast<float>(WeaponSheet->RateOfFire)
		: PrimaryComponentTick.TickInterval;
	if (const UWorld* World = GetWorld())
	{
		NextOpportunisticFireTime = World->GetTimeSeconds() + FMath::Max(BurstSeparation, MinimumRetryInterval);
	}

	if (!bTimedBurst)
	{
		HandleBurstFinished();
	}
	return true;
}

void UWeaponFiringComponent::ApplyBurstSeparation(AAgent* Agent, const float BurstSeparation)
{
	UAbilitySystemComponent* AbilitySystemComponent = Agent ? Agent->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent || BurstSeparation <= 0.0f)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(Agent);
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		UGE_BurstSeparation::StaticClass(),
		1.0f,
		EffectContext);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetDuration(BurstSeparation, true);
	SpecHandle.Data->DynamicGrantedTags.AddTag(TAG_State_Weapon_BurstSeparation.GetTag());
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UWeaponFiringComponent::HandleBurstFinished()
{
	const bool bWasDeliberate = bActiveBurstIsDeliberate;
	ActiveBurstWeapon.Reset();
	bBurstInProgress = false;
	bActiveBurstIsDeliberate = false;
	if (bWasDeliberate)
	{
		CompleteDeliberateFire(true);
	}
}

void UWeaponFiringComponent::CompleteDeliberateFire(const bool bSucceeded)
{
	UPlanner* Planner = DeliberatePlanner.Get();
	StopActiveBurst();
	DeliberatePlanner.Reset();
	DeliberateAlignmentDeadline = 0.0;
	if (Planner)
	{
		Planner->CompleteActiveAction(bSucceeded ? EActionResult::Succeeded : EActionResult::Failed);
	}
}

void UWeaponFiringComponent::StopActiveBurst()
{
	if (AWeapon* Weapon = ActiveBurstWeapon.Get())
	{
		Weapon->StopFireBurst();
	}
	ActiveBurstWeapon.Reset();
	bBurstInProgress = false;
	bActiveBurstIsDeliberate = false;
}
