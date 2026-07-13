// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GA_ThrowGrenade.cpp
 * @brief Implements the gameplay ability used to receive AI grenade throw data.
 */

#include "GA_ThrowGrenade.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Agent.h"
#include "../../AgentCustomization.h"
#include "Components/SkeletalMeshComponent.h"
#include "../../EquipmentComponent.h"
#include "../../EquipmentCustomization.h"
#include "../GameplayEffects/GE_BurstSeparation.h"
#include "../../PawnCustomization.h"
#include "../../Projectile.h"
#include "../../ScenarioForgeGameplayTags.h"

UGA_ThrowGrenade::UGA_ThrowGrenade()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ThrowReleaseEventTag = TAG_Event_Animation_ThrowGrenade_Release.GetTag();
}

void UGA_ThrowGrenade::SetPendingLaunchVelocity(const FVector& InLaunchVelocity)
{
	PendingLaunchVelocity = InLaunchVelocity;
	bHasPendingLaunchVelocity = true;
}

void UGA_ThrowGrenade::ClearPendingLaunchVelocity()
{
	PendingLaunchVelocity = FVector::ZeroVector;
	bHasPendingLaunchVelocity = false;
}

FTransform UGA_ThrowGrenade::GetGrenadeReleaseTransform() const
{
	const AAgent* Agent = Cast<AAgent>(GetAvatarActorFromActorInfo());
	return Agent ? Agent->GetGrenadeReleaseTransform() : FTransform::Identity;
}

void UGA_ThrowGrenade::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bReceivedThrowReleaseEvent = false;
	bSpawnedGrenadeProjectile = false;

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_ThrowGrenade::ActivateAbility - CommitAbility failed for %s."), *GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ApplyGrenadeThrowCooldown(ActorInfo);

	const AAgent* Agent = Cast<AAgent>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	const UPawnCustomization* PawnCustomization = Agent ? Agent->GetResolvedPawnCustomization() : nullptr;
	UAnimMontage* ThrowMontage = PawnCustomization ? PawnCustomization->ThrowGrenadeMontage : nullptr;
	if (!ThrowMontage)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UGA_ThrowGrenade::ActivateAbility - No ThrowGrenadeMontage on resolved pawn sheet for %s."),
			*GetNameSafe(Agent));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("UGA_ThrowGrenade::ActivateAbility - Activated for %s montage=%s waitingForReleaseEvent=%s pendingVelocity=%s."),
		*GetNameSafe(Agent),
		*GetNameSafe(ThrowMontage),
		*ThrowReleaseEventTag.ToString(),
		bHasPendingLaunchVelocity ? *PendingLaunchVelocity.ToCompactString() : TEXT("None"));

	ThrowReleaseEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ThrowReleaseEventTag,
		nullptr,
		false,
		true);
	if (ThrowReleaseEventTask)
	{
		ThrowReleaseEventTask->EventReceived.AddDynamic(this, &UGA_ThrowGrenade::HandleThrowReleaseEvent);
		ThrowReleaseEventTask->ReadyForActivation();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_ThrowGrenade::ActivateAbility - Failed to create WaitGameplayEvent task for %s on %s."), *ThrowReleaseEventTag.ToString(), *GetNameSafe(Agent));
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		ThrowMontage);
	if (!MontageTask)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_ThrowGrenade::ActivateAbility - Failed to create montage task for %s on %s."), *GetNameSafe(ThrowMontage), *GetNameSafe(Agent));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UGA_ThrowGrenade::HandleThrowMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_ThrowGrenade::HandleThrowMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_ThrowGrenade::HandleThrowMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_ThrowGrenade::HandleThrowMontageFinished);
	MontageTask->ReadyForActivation();
}

void UGA_ThrowGrenade::ApplyGrenadeThrowCooldown(const FGameplayAbilityActorInfo* ActorInfo) const
{
	AAgent* Agent = Cast<AAgent>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	UAbilitySystemComponent* AbilitySystemComponent = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const UEquipmentComponent* EquipmentComponent = Agent ? Agent->GetEquipmentComponent() : nullptr;
	const UAgentCustomization* AgentCustomization = Agent ? Agent->GetAgentCustomization() : nullptr;
	if (!Agent || !AbilitySystemComponent || !EquipmentComponent || !AgentCustomization)
	{
		return;
	}

	const FGrenadeProperties* GrenadeProperties = AgentCustomization->FindResolvedGrenadeProperties(EquipmentComponent->GetCurrentGrenadeType());
	const float ThrowCooldown = GrenadeProperties ? FMath::Max(0.0f, GrenadeProperties->GrenadeThrowDelay) : 0.0f;
	if (ThrowCooldown <= 0.0f)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(Agent);

	FGameplayEffectSpecHandle CooldownSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		UGE_BurstSeparation::StaticClass(),
		1.0f,
		EffectContext);

	if (!CooldownSpecHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_ThrowGrenade::ApplyGrenadeThrowCooldown - Failed to create grenade cooldown effect for %s."), *GetNameSafe(Agent));
		return;
	}

	CooldownSpecHandle.Data->SetDuration(ThrowCooldown, true);
	CooldownSpecHandle.Data->DynamicGrantedTags.AddTag(TAG_Cooldown_AI_ThrowGrenade.GetTag());
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*CooldownSpecHandle.Data.Get());
}

void UGA_ThrowGrenade::HandleGrenadeRelease_Implementation(const FGameplayEventData& EventData)
{
	AAgent* Agent = Cast<AAgent>(GetAvatarActorFromActorInfo());
	UEquipmentComponent* EquipmentComponent = Agent ? Agent->GetEquipmentComponent() : nullptr;
	UWorld* World = Agent ? Agent->GetWorld() : nullptr;
	if (!Agent || !EquipmentComponent || !World)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_ThrowGrenade::HandleGrenadeRelease - Cannot spawn grenade, missing agent/equipment/world. Agent=%s Equipment=%s World=%s."),
			*GetNameSafe(Agent),
			EquipmentComponent ? TEXT("valid") : TEXT("null"),
			World ? TEXT("valid") : TEXT("null"));
		return;
	}

	if (!bHasPendingLaunchVelocity)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_ThrowGrenade::HandleGrenadeRelease - Cannot spawn grenade for %s, no pending launch velocity."), *GetNameSafe(Agent));
		return;
	}

	const EGrenadeType GrenadeType = EquipmentComponent->GetCurrentGrenadeType();
	UEquipmentCustomization* GrenadeEquipment = EquipmentComponent->GetGrenadeEquipment(GrenadeType);
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("UGA_ThrowGrenade::HandleGrenadeRelease - Release event received for %s tag=%s grenadeType=%s equipment=%s pendingVelocity=%s."),
		*GetNameSafe(Agent),
		*EventData.EventTag.ToString(),
		*UEnum::GetValueAsString(GrenadeType),
		*GetNameSafe(GrenadeEquipment),
		*PendingLaunchVelocity.ToCompactString());

	if (!GrenadeEquipment
		|| GrenadeEquipment->Category != EEquipmentCategory::Grenade
		|| GrenadeEquipment->UseBehavior != EEquipmentUseBehavior::SpawnProjectile
		|| !GrenadeEquipment->ProjectileToSpawn)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UGA_ThrowGrenade::HandleGrenadeRelease - Cannot spawn grenade for %s type=%s, missing grenade equipment or projectile sheet."),
			*GetNameSafe(Agent),
			*UEnum::GetValueAsString(GrenadeType));
		return;
	}

	if (!EquipmentComponent->ConsumeGrenades(GrenadeType, 1))
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_ThrowGrenade::HandleGrenadeRelease - Cannot spawn grenade for %s, no %s grenades left."), *GetNameSafe(Agent), *UEnum::GetValueAsString(GrenadeType));
		return;
	}

	const FTransform ReleaseTransform = Agent->GetGrenadeReleaseTransform();
	const UPawnCustomization* PawnCustomization = Agent->GetResolvedPawnCustomization();
	const USkeletalMeshComponent* AgentMesh = Agent->GetMesh();
	const FName ReleaseSocketName = PawnCustomization ? PawnCustomization->GrenadeReleaseSocketName : NAME_None;
	const bool bHasReleaseSocket = AgentMesh && ReleaseSocketName != NAME_None && AgentMesh->DoesSocketExist(ReleaseSocketName);
	if (!bHasReleaseSocket)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UGA_ThrowGrenade::HandleGrenadeRelease - Grenade release socket '%s' was not found on mesh %s for %s; spawning from actor transform instead."),
			*ReleaseSocketName.ToString(),
			*GetNameSafe(AgentMesh ? AgentMesh->GetSkeletalMeshAsset() : nullptr),
			*GetNameSafe(Agent));
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Agent;
	SpawnParameters.Instigator = Agent;

	AProjectile* Projectile = World->SpawnActor<AProjectile>(AProjectile::StaticClass(), ReleaseTransform, SpawnParameters);
	if (!Projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_ThrowGrenade::HandleGrenadeRelease - Failed to spawn grenade projectile actor for %s."), *GetNameSafe(Agent));
		return;
	}

	Projectile->ApplyProjectileCustomization(GrenadeEquipment->ProjectileToSpawn);
	Projectile->Launch(PendingLaunchVelocity);
	bSpawnedGrenadeProjectile = true;

	ClearPendingLaunchVelocity();
}

void UGA_ThrowGrenade::HandleThrowMontageFinished()
{
	AAgent* Agent = Cast<AAgent>(GetAvatarActorFromActorInfo());
	if (!bReceivedThrowReleaseEvent)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UGA_ThrowGrenade::HandleThrowMontageFinished - Montage finished for %s but release event %s was never received. Check the montage/animation notify tag and that the notify exists on the montage being played."),
			*GetNameSafe(Agent),
			*ThrowReleaseEventTag.ToString());
	}
	else if (!bSpawnedGrenadeProjectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_ThrowGrenade::HandleThrowMontageFinished - Release event was received for %s but no grenade projectile spawned. Check warnings above for equipment, projectile sheet, grenade count, or spawn failure."), *GetNameSafe(Agent));
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UGA_ThrowGrenade::HandleThrowReleaseEvent(FGameplayEventData Payload)
{
	bReceivedThrowReleaseEvent = true;
	HandleGrenadeRelease(Payload);
}
