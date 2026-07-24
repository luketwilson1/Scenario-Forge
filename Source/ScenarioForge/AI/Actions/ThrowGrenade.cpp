// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ThrowGrenade.cpp
 * @brief Implements the GOAP action that activates the grenade throw ability.
 */

#include "ThrowGrenade.h"

#include "AbilitySystemComponent.h"
#include "Agent.h"
#include "AgentAIController.h"
#include "AgentSheet.h"
#include "Planner.h"
#include "EquipmentComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameplayAbilitySystem/Abilities/GA_ThrowGrenade.h"
#include "GrenadeThrowFunctionLibrary.h"
#include "ScenarioForgeGameplayTags.h"

namespace
{
	UGA_ThrowGrenade* GetThrowGrenadeAbilityInstance(FGameplayAbilitySpec& AbilitySpec)
	{
		if (UGA_ThrowGrenade* AbilityInstance = Cast<UGA_ThrowGrenade>(AbilitySpec.GetPrimaryInstance()))
		{
			return AbilityInstance;
		}

		return Cast<UGA_ThrowGrenade>(AbilitySpec.Ability);
	}

	bool FindThrowGrenadeAbility(UAbilitySystemComponent& AbilitySystemComponent, FGameplayAbilitySpecHandle& OutAbilityHandle, UGA_ThrowGrenade*& OutAbilityInstance)
	{
		for (FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent.GetActivatableAbilities())
		{
			if (!AbilitySpec.Ability || !AbilitySpec.Ability->IsA(UGA_ThrowGrenade::StaticClass()))
			{
				continue;
			}

			OutAbilityHandle = AbilitySpec.Handle;
			OutAbilityInstance = GetThrowGrenadeAbilityInstance(AbilitySpec);
			return OutAbilityHandle.IsValid() && OutAbilityInstance;
		}

		return false;
	}
}

/**
 * @brief Configures grenade planning preconditions, effects, and cooldown constraints.
 */
UThrowGrenade::UThrowGrenade()
{
	TruePreconditions.AddTag(TAG_State_CanThrowGrenade.GetTag());
	TruePreconditions.AddTag(TAG_State_TargetsGrouped.GetTag());
	AddedEffects.AddTag(TAG_State_DestroyTarget.GetTag());
	RemovedEffects.AddTag(TAG_State_GrenadeNear.GetTag());
}

bool UThrowGrenade::GetCachedTargetLocation(const AAgentAIController& Controller, FVector& OutTargetLocation) const
{
	return Controller.GetCurrentGrenadeTargetLocation(OutTargetLocation);
}

bool UThrowGrenade::GetCachedThrowSolution(const AAgentAIController& Controller, FGrenadeThrowSolution& OutSolution) const
{
	return Controller.GetCurrentGrenadeThrowSolution(OutSolution);
}

/**
 * @brief Activates the owning agent's grenade ability with the calculated throw solution.
 *
 * @param Planner Planner executing the grenade throw action.
 * @return Running after activation, Failed when the throw cannot start, or Invalid without a planner.
 */
EActionResult UThrowGrenade::Execute(UPlanner* Planner)
{
	if (!Planner)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThrowGrenadeAction: blocked, missing planner."));
		return EActionResult::Invalid;
	}

	AAgentAIController* AgentAIController = Cast<AAgentAIController>(Planner->GetOwner());
	AAgent* OwningAgent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	UAgentSheet* AgentSheet = OwningAgent ? OwningAgent->GetAgentSheet() : nullptr;
	UEquipmentComponent* EquipmentComponent = OwningAgent ? OwningAgent->GetEquipmentComponent() : nullptr;
	UAbilitySystemComponent* AbilitySystemComponent = OwningAgent ? OwningAgent->GetAbilitySystemComponent() : nullptr;
	if (!AgentAIController
		|| !OwningAgent
		|| !AgentSheet
		|| !EquipmentComponent
		|| !EquipmentComponent->RefreshCurrentGrenadeType()
		|| !EquipmentComponent->HasAnyGrenade()
		|| !AbilitySystemComponent)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("ThrowGrenadeAction[%s]: blocked controller=%s agent=%s sheet=%s equipment=%s hasAnyGrenade=%s ASC=%s."),
			*GetNameSafe(OwningAgent),
			AgentAIController ? TEXT("true") : TEXT("false"),
			OwningAgent ? TEXT("true") : TEXT("false"),
			AgentSheet ? TEXT("true") : TEXT("false"),
			EquipmentComponent ? TEXT("true") : TEXT("false"),
			(EquipmentComponent && EquipmentComponent->HasAnyGrenade()) ? TEXT("true") : TEXT("false"),
			AbilitySystemComponent ? TEXT("true") : TEXT("false"));
		return EActionResult::Failed;
	}

	FGrenadeThrowSolution ThrowSolution;
	if (!GetCachedThrowSolution(*AgentAIController, ThrowSolution))
	{
		FVector TargetLocation = FVector::ZeroVector;
		if (!GetCachedTargetLocation(*AgentAIController, TargetLocation))
		{
			UE_LOG(LogTemp, Warning, TEXT("ThrowGrenadeAction[%s]: blocked, no cached grenade target location."), *GetNameSafe(OwningAgent));
			return EActionResult::Failed;
		}

		const FGrenadeProperties* GrenadeProperties = AgentSheet->FindResolvedGrenadeProperties(EquipmentComponent->GetCurrentGrenadeType());
		if (!GrenadeProperties
			|| !UGrenadeThrowFunctionLibrary::BuildThrowSolutionForAgent(OwningAgent, TargetLocation, *GrenadeProperties, ThrowSolution)
			|| !ThrowSolution.bVelocityInRange
			|| ThrowSolution.bTrajectoryBlocked)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("ThrowGrenadeAction[%s]: blocked, fallback solution failed properties=%s velocityInRange=%s trajectoryBlocked=%s target=%s."),
				*GetNameSafe(OwningAgent),
				GrenadeProperties ? TEXT("true") : TEXT("false"),
				ThrowSolution.bVelocityInRange ? TEXT("true") : TEXT("false"),
				ThrowSolution.bTrajectoryBlocked ? TEXT("true") : TEXT("false"),
				*TargetLocation.ToCompactString());
			return EActionResult::Failed;
		}
	}

	FGameplayAbilitySpecHandle GrenadeAbilityHandle;
	UGA_ThrowGrenade* GrenadeAbilityInstance = nullptr;
	if (!FindThrowGrenadeAbility(*AbilitySystemComponent, GrenadeAbilityHandle, GrenadeAbilityInstance))
	{
		UE_LOG(LogTemp, Warning, TEXT("ThrowGrenadeAction[%s]: blocked, UGA_ThrowGrenade is not granted to this ability system."), *GetNameSafe(OwningAgent));
		return EActionResult::Failed;
	}

	GrenadeAbilityInstance->SetPendingLaunchVelocity(ThrowSolution.LaunchVelocity);
	if (!AbilitySystemComponent->TryActivateAbility(GrenadeAbilityHandle))
	{
		UE_LOG(LogTemp, Warning, TEXT("ThrowGrenadeAction[%s]: TryActivateAbility failed."), *GetNameSafe(OwningAgent));
		GrenadeAbilityInstance->ClearPendingLaunchVelocity();
		return EActionResult::Failed;
	}

	return EActionResult::Running;
}
