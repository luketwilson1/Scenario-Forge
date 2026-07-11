// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file BA_ThrowGrenade.cpp
 * @brief Implements the GOAP behavior action that activates the grenade throw ability.
 */

#include "BA_ThrowGrenade.h"

#include "AbilitySystemComponent.h"
#include "Agent.h"
#include "AgentAIController.h"
#include "AgentCustomization.h"
#include "DecisionComponent.h"
#include "EquipmentComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameplayAbilitySystem/Abilities/GA_ThrowGrenade.h"
#include "GrenadeThrowFunctionLibrary.h"

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

void UBA_ThrowGrenade::Execute_Implementation(UDecisionComponent* Agent, const UActionDefinition* ActionDefinition)
{
	(void)ActionDefinition;

	if (!Agent)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThrowGrenadeAction: blocked, missing decision component."));
		return;
	}

	AAgentAIController* AgentAIController = Cast<AAgentAIController>(Agent->GetOwner());
	AAgent* OwningAgent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	UAgentCustomization* AgentCustomization = OwningAgent ? OwningAgent->GetAgentCustomization() : nullptr;
	UEquipmentComponent* EquipmentComponent = OwningAgent ? OwningAgent->GetEquipmentComponent() : nullptr;
	UAbilitySystemComponent* AbilitySystemComponent = OwningAgent ? OwningAgent->GetAbilitySystemComponent() : nullptr;
	if (!AgentAIController
		|| !OwningAgent
		|| !AgentCustomization
		|| !EquipmentComponent
		|| !EquipmentComponent->RefreshCurrentGrenadeType()
		|| !EquipmentComponent->HasAnyGrenade()
		|| !AbilitySystemComponent)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("ThrowGrenadeAction[%s]: blocked controller=%s agent=%s customization=%s equipment=%s hasAnyGrenade=%s ASC=%s."),
			*GetNameSafe(OwningAgent),
			AgentAIController ? TEXT("true") : TEXT("false"),
			OwningAgent ? TEXT("true") : TEXT("false"),
			AgentCustomization ? TEXT("true") : TEXT("false"),
			EquipmentComponent ? TEXT("true") : TEXT("false"),
			(EquipmentComponent && EquipmentComponent->HasAnyGrenade()) ? TEXT("true") : TEXT("false"),
			AbilitySystemComponent ? TEXT("true") : TEXT("false"));
		return;
	}

	FGrenadeThrowSolution ThrowSolution;
	if (!AgentAIController->GetCurrentGrenadeThrowSolution(ThrowSolution))
	{
		FVector TargetLocation = FVector::ZeroVector;
		if (!AgentAIController->GetCurrentGrenadeTargetLocation(TargetLocation))
		{
			UE_LOG(LogTemp, Warning, TEXT("ThrowGrenadeAction[%s]: blocked, no cached grenade target location."), *GetNameSafe(OwningAgent));
			return;
		}

		const FGrenadeProperties* GrenadeProperties = AgentCustomization->FindResolvedGrenadeProperties(EquipmentComponent->GetCurrentGrenadeType());
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
			return;
		}
	}

	FGameplayAbilitySpecHandle GrenadeAbilityHandle;
	UGA_ThrowGrenade* GrenadeAbilityInstance = nullptr;
	if (!FindThrowGrenadeAbility(*AbilitySystemComponent, GrenadeAbilityHandle, GrenadeAbilityInstance))
	{
		UE_LOG(LogTemp, Warning, TEXT("ThrowGrenadeAction[%s]: blocked, UGA_ThrowGrenade is not granted to this ability system."), *GetNameSafe(OwningAgent));
		return;
	}

	GrenadeAbilityInstance->SetPendingLaunchVelocity(ThrowSolution.LaunchVelocity);
	if (!AbilitySystemComponent->TryActivateAbility(GrenadeAbilityHandle))
	{
		UE_LOG(LogTemp, Warning, TEXT("ThrowGrenadeAction[%s]: TryActivateAbility failed."), *GetNameSafe(OwningAgent));
		GrenadeAbilityInstance->ClearPendingLaunchVelocity();
		return;
	}
}
