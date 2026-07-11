// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AnimNotify_GameplayEvent.h
 * @brief Declares a generic animation notify that sends a gameplay event to the animated actor's ability system.
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_GameplayEvent.generated.h"

/**
 * @brief Sends a gameplay event tag from an animation frame to the owning actor's Ability System Component.
 */
UCLASS(meta = (DisplayName = "Scenario Forge Gameplay Event"))
class SCENARIOFORGE_API UAnimNotify_GameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** Gameplay event tag sent when this notify fires. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event")
	FGameplayTag EventTag;

	/** Optional payload magnitude sent with the gameplay event. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event")
	float EventMagnitude = 0.0f;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
