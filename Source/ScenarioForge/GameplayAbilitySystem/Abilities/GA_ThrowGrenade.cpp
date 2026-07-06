// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GA_ThrowGrenade.cpp
 * @brief Implements the gameplay ability used to receive AI grenade throw data.
 */

#include "GA_ThrowGrenade.h"

UGA_ThrowGrenade::UGA_ThrowGrenade()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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
