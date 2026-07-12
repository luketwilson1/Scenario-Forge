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

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_ThrowGrenade[%s]: CommitAbility failed."), *GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr));
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
			TEXT("GA_ThrowGrenade[%s]: no ThrowGrenadeMontage on resolved pawn sheet."),
			*GetNameSafe(Agent));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

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

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		ThrowMontage);
	if (!MontageTask)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_ThrowGrenade[%s]: failed to create montage task for %s."), *GetNameSafe(Agent), *GetNameSafe(ThrowMontage));
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
		UE_LOG(LogTemp, Warning, TEXT("GA_ThrowGrenade[%s]: failed to create grenade cooldown effect."), *GetNameSafe(Agent));
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
		UE_LOG(LogTemp, Warning, TEXT("GA_ThrowGrenade[%s]: cannot spawn grenade, missing agent/equipment/world."), *GetNameSafe(Agent));
		return;
	}

	if (!bHasPendingLaunchVelocity)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_ThrowGrenade[%s]: cannot spawn grenade, no pending launch velocity."), *GetNameSafe(Agent));
		return;
	}

	const EGrenadeType GrenadeType = EquipmentComponent->GetCurrentGrenadeType();
	UEquipmentCustomization* GrenadeEquipment = EquipmentComponent->GetGrenadeEquipment(GrenadeType);
	if (!GrenadeEquipment
		|| GrenadeEquipment->Category != EEquipmentCategory::Grenade
		|| GrenadeEquipment->UseBehavior != EEquipmentUseBehavior::SpawnProjectile
		|| !GrenadeEquipment->ProjectileToSpawn)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("GA_ThrowGrenade[%s]: cannot spawn grenade type %s, missing grenade equipment or projectile sheet."),
			*GetNameSafe(Agent),
			*UEnum::GetValueAsString(GrenadeType));
		return;
	}

	if (!EquipmentComponent->ConsumeGrenades(GrenadeType, 1))
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_ThrowGrenade[%s]: cannot spawn grenade, no %s grenades left."), *GetNameSafe(Agent), *UEnum::GetValueAsString(GrenadeType));
		return;
	}

	const FTransform ReleaseTransform = Agent->GetGrenadeReleaseTransform();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Agent;
	SpawnParameters.Instigator = Agent;

	AProjectile* Projectile = World->SpawnActor<AProjectile>(AProjectile::StaticClass(), ReleaseTransform, SpawnParameters);
	if (!Projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_ThrowGrenade[%s]: failed to spawn grenade projectile actor."), *GetNameSafe(Agent));
		return;
	}

	Projectile->ApplyProjectileCustomization(GrenadeEquipment->ProjectileToSpawn);
	Projectile->Launch(PendingLaunchVelocity);

	ClearPendingLaunchVelocity();
}

void UGA_ThrowGrenade::HandleThrowMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UGA_ThrowGrenade::HandleThrowReleaseEvent(FGameplayEventData Payload)
{
	HandleGrenadeRelease(Payload);
}
