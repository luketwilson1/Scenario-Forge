// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AnimNotify_GameplayEvent.cpp
 * @brief Implements gameplay event dispatch from animation notifies.
 */

#include "AnimNotify_GameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_GameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp || !EventTag.IsValid())
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = Owner;
	EventData.Target = Owner;
	EventData.EventMagnitude = EventMagnitude;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, EventData);
}
