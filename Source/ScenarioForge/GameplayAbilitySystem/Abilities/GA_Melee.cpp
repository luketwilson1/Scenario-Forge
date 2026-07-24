// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GA_Melee.cpp
 * @brief Implements montage playback and authoritative melee damage traces.
 */

#include "GA_Melee.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Agent.h"
#include "AgentSheet.h"
#include "AI/Actions/Action.h"
#include "AI/AgentAIController.h"
#include "AI/Planner.h"
#include "Components/SkeletalMeshComponent.h"
#include "DamageEffectSheet.h"
#include "PawnSheet.h"
#include "ScenarioForgeGameplayTags.h"

namespace
{
	void CompleteMeleePlannerAction(AAgent* Agent, const EActionResult Result)
	{
		AAgentAIController* Controller = Agent ? Cast<AAgentAIController>(Agent->GetController()) : nullptr;
		if (UPlanner* Planner = Controller ? Controller->GetPlanner() : nullptr)
		{
			Planner->CompleteActiveAction(Result);
		}
	}
}

UGA_Melee::UGA_Melee()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationBlockedTags.AddTag(TAG_State_Downed.GetTag());
}

void UGA_Melee::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	HitAgents.Reset();

	AAgent* Agent = Cast<AAgent>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	const UPawnSheet* PawnSheet = Agent ? Agent->GetResolvedPawnSheet() : nullptr;
	UAnimMontage* MeleeMontage = PawnSheet ? PawnSheet->MeleeMontage : nullptr;
	if (!Agent || !MeleeMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_Melee[%s]: missing melee montage or ability commit failed."), *GetNameSafe(Agent));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		CompleteMeleePlannerAction(Agent, EActionResult::Failed);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MeleeMontage);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		CompleteMeleePlannerAction(Agent, EActionResult::Failed);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UGA_Melee::HandleMeleeMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Melee::HandleMeleeMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Melee::HandleMeleeMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Melee::HandleMeleeMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void UGA_Melee::PerformMeleeTrace(
	USkeletalMeshComponent* MeshComponent,
	const FName StartBoneName,
	const FName EndBoneName,
	const float SphereRadius)
{
	AAgent* SourceAgent = Cast<AAgent>(GetAvatarActorFromActorInfo());
	UWorld* World = SourceAgent ? SourceAgent->GetWorld() : nullptr;
	const UAgentSheet* SourceSheet = SourceAgent ? SourceAgent->GetAgentSheet() : nullptr;
	if (!IsActive()
		|| !SourceAgent
		|| !SourceAgent->HasAuthority()
		|| !World
		|| !MeshComponent
		|| MeshComponent->GetOwner() != SourceAgent
		|| !SourceSheet
		|| SphereRadius <= 0.0f
		|| !MeshComponent->DoesSocketExist(StartBoneName)
		|| !MeshComponent->DoesSocketExist(EndBoneName))
	{
		return;
	}

	const FMeleeProperties& MeleeProperties = SourceSheet->GetResolvedMeleeProperties();
	if (!MeleeProperties.DamageEffect)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeleeTrace), false, SourceAgent);
	TArray<FHitResult> HitResults;
	World->SweepMultiByObjectType(
		HitResults,
		MeshComponent->GetSocketLocation(StartBoneName),
		MeshComponent->GetSocketLocation(EndBoneName),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(SphereRadius),
		QueryParams);

	for (const FHitResult& HitResult : HitResults)
	{
		AAgent* HitAgent = Cast<AAgent>(HitResult.GetActor());
		if (!HitAgent
			|| HitAgent == SourceAgent
			|| HitAgent->IsDead()
			|| HitAgents.Contains(HitAgent)
			|| HitAgent->GetResolvedFaction() == SourceAgent->GetResolvedFaction())
		{
			continue;
		}

		HitAgents.Add(HitAgent);
		HitAgent->ApplyDamage(MeleeProperties.DamageEffect->GetDamageAmount(), SourceAgent);
	}
}

void UGA_Melee::HandleMeleeMontageCompleted()
{
	AAgent* Agent = Cast<AAgent>(GetAvatarActorFromActorInfo());
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
	CompleteMeleePlannerAction(Agent, EActionResult::Succeeded);
}

void UGA_Melee::HandleMeleeMontageInterrupted()
{
	AAgent* Agent = Cast<AAgent>(GetAvatarActorFromActorInfo());
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	CompleteMeleePlannerAction(Agent, EActionResult::Interrupted);
}
