// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AnimNotifyState_MeleeTrace.cpp
 * @brief Implements melee animation-window trace forwarding.
 */

#include "AnimNotifyState_MeleeTrace.h"

#include "AbilitySystemComponent.h"
#include "Agent.h"
#include "DrawDebugHelpers.h"
#include "GameplayAbilitySpec.h"
#include "GameplayAbilitySystem/Abilities/GA_Melee.h"

void UAnimNotifyState_MeleeTrace::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	PerformTrace(MeshComp);
}

void UAnimNotifyState_MeleeTrace::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	PerformTrace(MeshComp);
}

FString UAnimNotifyState_MeleeTrace::GetNotifyName_Implementation() const
{
	return TEXT("Melee Trace");
}

void UAnimNotifyState_MeleeTrace::PerformTrace(USkeletalMeshComponent* MeshComp) const
{
	if (bDrawDebugTrace)
	{
		DrawTraceDebug(MeshComp);
	}

	AAgent* Agent = MeshComp ? Cast<AAgent>(MeshComp->GetOwner()) : nullptr;
	UAbilitySystemComponent* AbilitySystemComponent = Agent ? Agent->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability || !AbilitySpec.Ability->IsA(UGA_Melee::StaticClass()))
		{
			continue;
		}

		UGA_Melee* MeleeAbility = Cast<UGA_Melee>(AbilitySpec.GetPrimaryInstance());
		if (MeleeAbility && MeleeAbility->IsActive())
		{
			MeleeAbility->PerformMeleeTrace(MeshComp, StartBoneName, EndBoneName, SphereRadius);
		}
		return;
	}
}

void UAnimNotifyState_MeleeTrace::DrawTraceDebug(USkeletalMeshComponent* MeshComp) const
{
	UWorld* World = MeshComp ? MeshComp->GetWorld() : nullptr;
	if (!World
		|| SphereRadius <= 0.0f
		|| !MeshComp->DoesSocketExist(StartBoneName)
		|| !MeshComp->DoesSocketExist(EndBoneName))
	{
		return;
	}

	const FVector StartLocation = MeshComp->GetSocketLocation(StartBoneName);
	const FVector EndLocation = MeshComp->GetSocketLocation(EndBoneName);
	const FVector TraceDelta = EndLocation - StartLocation;
	const float TraceLength = TraceDelta.Size();
	constexpr float DebugDuration = 0.05f;
	if (TraceLength <= KINDA_SMALL_NUMBER)
	{
		DrawDebugSphere(World, StartLocation, SphereRadius, 16, FColor::Red, false, DebugDuration);
		return;
	}

	DrawDebugCapsule(
		World,
		(StartLocation + EndLocation) * 0.5f,
		TraceLength * 0.5f + SphereRadius,
		SphereRadius,
		FRotationMatrix::MakeFromZ(TraceDelta).ToQuat(),
		FColor::Red,
		false,
		DebugDuration);
}
