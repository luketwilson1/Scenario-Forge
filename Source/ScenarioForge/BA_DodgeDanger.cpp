// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file BA_DodgeDanger.cpp
 * @brief Implements the GOAP danger dodge behavior.
 */

#include "BA_DodgeDanger.h"

#include "AIController.h"
#include "Agent.h"
#include "AgentAIController.h"
#include "AgentCustomization.h"
#include "DecisionComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "ScenarioForgeGameplayTags.h"
#include "TimerManager.h"

namespace
{
	constexpr float DodgeAcceptanceRadius = 25.0f;
	constexpr float MinimumDodgeDuration = 0.1f;

	FVector ResolveDodgeDirection(const AAgent& Agent, const AAgentAIController& AgentAIController)
	{
		FVector DangerLocation = FVector::ZeroVector;
		if (Agent.GetCurrentDangerSourceLocation(DangerLocation))
		{
			const FVector AwayFromDanger = Agent.GetActorLocation() - DangerLocation;
			if (!AwayFromDanger.IsNearlyZero())
			{
				return AwayFromDanger.GetSafeNormal2D();
			}
		}

		if (const AActor* TargetActor = AgentAIController.GetCurrentEnemyTarget())
		{
			const FVector AwayFromTarget = Agent.GetActorLocation() - TargetActor->GetActorLocation();
			if (!AwayFromTarget.IsNearlyZero())
			{
				return AwayFromTarget.GetSafeNormal2D();
			}
		}

		return Agent.GetActorRightVector().GetSafeNormal2D();
	}

	FVector ProjectDodgeDestination(const AAgent& Agent, const FVector& DesiredDestination)
	{
		UWorld* World = Agent.GetWorld();
		UNavigationSystemV1* NavigationSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
		if (!NavigationSystem)
		{
			return DesiredDestination;
		}

		FNavLocation ProjectedLocation;
		return NavigationSystem->ProjectPointToNavigation(DesiredDestination, ProjectedLocation, FVector(100.0f, 100.0f, 250.0f))
			? ProjectedLocation.Location
			: DesiredDestination;
	}
}

void UBA_DodgeDanger::Execute_Implementation(UDecisionComponent* Agent, const UActionDefinition* ActionDefinition)
{
	(void)ActionDefinition;

	if (!Agent)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeDangerAction: blocked, missing decision component."));
		return;
	}

	AAgentAIController* AgentAIController = Cast<AAgentAIController>(Agent->GetOwner());
	AAgent* OwningAgent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	const UAgentCustomization* AgentCustomization = OwningAgent ? OwningAgent->GetAgentCustomization() : nullptr;
	if (!AgentAIController || !OwningAgent || !AgentCustomization)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeDangerAction: blocked, controller=%s agent=%s customization=%s."),
			AgentAIController ? TEXT("true") : TEXT("false"),
			OwningAgent ? TEXT("true") : TEXT("false"),
			AgentCustomization ? TEXT("true") : TEXT("false"));
		return;
	}

	const FDodgeProperties& DodgeProperties = AgentCustomization->GetResolvedDodgeProperties();
	const float DodgeDistance = FMath::Max(0.0f, DodgeProperties.Distance);
	const float DodgeSpeed = FMath::Max(0.0f, DodgeProperties.Speed);
	const float ReactionDelay = FMath::Max(0.0f, DodgeProperties.ReactionDelay);
	if (DodgeDistance <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeDangerAction[%s]: blocked, dodge distance is 0."), *GetNameSafe(OwningAgent));
		return;
	}

	UWorld* World = OwningAgent->GetWorld();
	if (!World)
	{
		return;
	}

	const TWeakObjectPtr<UDecisionComponent> WeakDecisionComponent = Agent;
	const TWeakObjectPtr<AAgentAIController> WeakController = AgentAIController;
	const TWeakObjectPtr<AAgent> WeakAgent = OwningAgent;
	const float MovementDuration = DodgeSpeed > 0.0f
		? FMath::Max(MinimumDodgeDuration, DodgeDistance / DodgeSpeed)
		: MinimumDodgeDuration;

	FTimerDelegate StartDodgeDelegate;
	StartDodgeDelegate.BindLambda([WeakDecisionComponent, WeakController, WeakAgent, DodgeDistance, DodgeSpeed, MovementDuration]()
	{
		UDecisionComponent* DecisionComponent = WeakDecisionComponent.Get();
		AAgentAIController* Controller = WeakController.Get();
		AAgent* AgentPawn = WeakAgent.Get();
		if (!DecisionComponent || !Controller || !AgentPawn)
		{
			return;
		}

		const FVector DodgeDirection = ResolveDodgeDirection(*AgentPawn, *Controller);
		if (DodgeDirection.IsNearlyZero())
		{
			return;
		}

		UFloatingPawnMovement* FloatingMovement = Cast<UFloatingPawnMovement>(AgentPawn->GetMovementComponent());
		const float PreviousMaxSpeed = FloatingMovement ? FloatingMovement->MaxSpeed : 0.0f;
		if (FloatingMovement && DodgeSpeed > 0.0f)
		{
			FloatingMovement->MaxSpeed = DodgeSpeed;
		}

		AgentPawn->SetDodgeDirectionWorld(DodgeDirection);
		DecisionComponent->AddCurrentState(TAG_State_Dodging.GetTag());

		const FVector DesiredDestination = AgentPawn->GetActorLocation() + DodgeDirection * DodgeDistance;
		const FVector DodgeDestination = ProjectDodgeDestination(*AgentPawn, DesiredDestination);
		const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
			DodgeDestination,
			DodgeAcceptanceRadius,
			true,
			true,
			true,
			true);

		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			if (FloatingMovement && DodgeSpeed > 0.0f)
			{
				FloatingMovement->MaxSpeed = PreviousMaxSpeed;
			}
			AgentPawn->ClearDodgeDirection();
			DecisionComponent->RemoveCurrentState(TAG_State_Dodging.GetTag());
			return;
		}

		if (UWorld* World = AgentPawn->GetWorld())
		{
			FTimerDelegate FinishDodgeDelegate;
			FinishDodgeDelegate.BindLambda([WeakDecisionComponent, WeakController, WeakAgent, PreviousMaxSpeed, DodgeSpeed]()
			{
				UDecisionComponent* DecisionComponent = WeakDecisionComponent.Get();
				AAgentAIController* Controller = WeakController.Get();
				AAgent* AgentPawn = WeakAgent.Get();
				if (Controller)
				{
					Controller->StopMovement();
				}
				if (AgentPawn)
				{
					if (UFloatingPawnMovement* FloatingMovement = Cast<UFloatingPawnMovement>(AgentPawn->GetMovementComponent()))
					{
						if (DodgeSpeed > 0.0f)
						{
							FloatingMovement->MaxSpeed = PreviousMaxSpeed;
						}
					}
					AgentPawn->ClearDodgeDirection();
				}
				if (DecisionComponent)
				{
					DecisionComponent->RemoveCurrentState(TAG_State_Dodging.GetTag());
				}
			});

			FTimerHandle FinishDodgeTimerHandle;
			World->GetTimerManager().SetTimer(FinishDodgeTimerHandle, FinishDodgeDelegate, MovementDuration, false);
		}
	});

	FTimerHandle StartDodgeTimerHandle;
	if (ReactionDelay <= 0.0f)
	{
		StartDodgeDelegate.ExecuteIfBound();
	}
	else
	{
		World->GetTimerManager().SetTimer(StartDodgeTimerHandle, StartDodgeDelegate, ReactionDelay, false);
	}
}
