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
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "ScenarioForgeGameplayTags.h"
#include "TimerManager.h"

namespace
{
	constexpr float DodgeAcceptanceRadius = 25.0f;
	constexpr float DodgeDebugLifetime = 3.0f;
	constexpr float DodgeMovementTickRate = 1.0f / 60.0f;
	constexpr float MinimumDodgeDuration = 0.1f;
	constexpr float WalkableSurfaceNormalZ = 0.7f;

	struct FDodgeCandidate
	{
		FVector Direction = FVector::ZeroVector;
		FVector Destination = FVector::ZeroVector;
		float Score = 0.0f;
		bool bValid = false;
	};

	FVector ResolveBaseDodgeDirection(const AAgent& Agent, const AAgentAIController& AgentAIController)
	{
		(void)AgentAIController;

		return Agent.GetActorForwardVector().GetSafeNormal2D();
	}

	bool ProjectDodgeDestination(const AAgent& Agent, const FVector& DesiredDestination, FVector& OutProjectedDestination)
	{
		UWorld* World = Agent.GetWorld();
		UNavigationSystemV1* NavigationSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
		if (!NavigationSystem)
		{
			OutProjectedDestination = DesiredDestination;
			return true;
		}

		FNavLocation ProjectedLocation;
		if (!NavigationSystem->ProjectPointToNavigation(DesiredDestination, ProjectedLocation, FVector(100.0f, 100.0f, 250.0f)))
		{
			return false;
		}

		OutProjectedDestination = ProjectedLocation.Location;
		return true;
	}

	FVector RotateDirection2D(const FVector& Direction, float AngleDegrees)
	{
		return Direction.RotateAngleAxis(AngleDegrees, FVector::UpVector).GetSafeNormal2D();
	}

	bool CapsuleSweepToDestination(const AAgent& Agent, const FVector& Destination, FHitResult& OutBlockingHit)
	{
		UWorld* World = Agent.GetWorld();
		const UCapsuleComponent* CapsuleComponent = Agent.GetCapsuleComponent();
		if (!World || !CapsuleComponent)
		{
			return false;
		}

		const FVector Start = Agent.GetActorLocation();
		const FVector End = FVector(Destination.X, Destination.Y, Start.Z);
		const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
			CapsuleComponent->GetScaledCapsuleRadius(),
			CapsuleComponent->GetScaledCapsuleHalfHeight());

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DodgeDangerSweep), false);
		QueryParams.AddIgnoredActor(&Agent);

		TArray<FHitResult> Hits;
		const bool bHitSomething = World->SweepMultiByChannel(
			Hits,
			Start,
			End,
			FQuat::Identity,
			ECC_Pawn,
			CapsuleShape,
			QueryParams);

		if (!bHitSomething)
		{
			return true;
		}

		for (const FHitResult& Hit : Hits)
		{
			if (!Hit.bBlockingHit)
			{
				continue;
			}

			// Horizontal capsule sweeps can report the walkable floor the pawn is already standing on.
			// Floor contact should not invalidate a dodge; walls and steep blocking surfaces should.
			if (Hit.ImpactNormal.Z >= WalkableSurfaceNormalZ || Hit.Normal.Z >= WalkableSurfaceNormalZ)
			{
				continue;
			}

			OutBlockingHit = Hit;
			return false;
		}

		return true;
	}

	float ScoreDodgeCandidate(const AAgent& Agent, const FVector& Direction, const FVector& Destination, const FVector& BaseDirection)
	{
		float Score = FVector::DotProduct(Direction, BaseDirection) * 100.0f;

		FVector DangerLocation = FVector::ZeroVector;
		if (Agent.GetCurrentDangerSourceLocation(DangerLocation))
		{
			Score += FVector::DistSquared2D(Destination, DangerLocation) * 0.001f;
		}

		return Score;
	}

	void DrawDodgeCandidateDebug(const AAgent& Agent, const FVector& Destination, const FColor& Color)
	{
		UWorld* World = Agent.GetWorld();
		const UCapsuleComponent* CapsuleComponent = Agent.GetCapsuleComponent();
		if (!World || !CapsuleComponent)
		{
			return;
		}

		const FVector Start = Agent.GetActorLocation();
		const FVector End = FVector(Destination.X, Destination.Y, Start.Z);
		DrawDebugLine(World, Start, End, Color, false, DodgeDebugLifetime, 0, 3.0f);
		DrawDebugCapsule(
			World,
			End,
			CapsuleComponent->GetScaledCapsuleHalfHeight(),
			CapsuleComponent->GetScaledCapsuleRadius(),
			FQuat::Identity,
			Color,
			false,
			DodgeDebugLifetime,
			0,
			2.0f);
	}

	void DrawRejectedDodgeCandidateDebug(const AAgent& Agent, const FVector& Destination, const FHitResult* BlockingHit)
	{
		DrawDodgeCandidateDebug(Agent, Destination, FColor::Red);

		UWorld* World = Agent.GetWorld();
		if (!World || !BlockingHit)
		{
			return;
		}

		const FString HitLabel = FString::Printf(
			TEXT("Blocked: %s.%s"),
			*GetNameSafe(BlockingHit->GetActor()),
			*GetNameSafe(BlockingHit->GetComponent()));
		DrawDebugString(
			World,
			BlockingHit->ImpactPoint + FVector(0.0f, 0.0f, 40.0f),
			HitLabel,
			nullptr,
			FColor::Red,
			DodgeDebugLifetime,
			true);
	}

	bool FindBestDodgeCandidate(const AAgent& Agent, const AAgentAIController& AgentAIController, float DodgeDistance, FDodgeCandidate& OutCandidate)
	{
		const FVector BaseDirection = ResolveBaseDodgeDirection(Agent, AgentAIController);
		if (BaseDirection.IsNearlyZero())
		{
			return false;
		}

		static constexpr float CandidateAngles[] = { 0.0f, 45.0f, -45.0f, 90.0f, -90.0f };
		bool bFoundValidCandidate = false;
		FDodgeCandidate BestCandidate;

		for (const float CandidateAngle : CandidateAngles)
		{
			const FVector Direction = RotateDirection2D(BaseDirection, CandidateAngle);
			const FVector DesiredDestination = Agent.GetActorLocation() + Direction * DodgeDistance;
			FVector ProjectedDestination = FVector::ZeroVector;
			if (!ProjectDodgeDestination(Agent, DesiredDestination, ProjectedDestination))
			{
				DrawRejectedDodgeCandidateDebug(Agent, DesiredDestination, nullptr);
				continue;
			}

			FHitResult SweepHit;
			if (!CapsuleSweepToDestination(Agent, ProjectedDestination, SweepHit))
			{
				DrawRejectedDodgeCandidateDebug(Agent, ProjectedDestination, &SweepHit);
				continue;
			}

			FDodgeCandidate Candidate;
			Candidate.Direction = Direction;
			Candidate.Destination = ProjectedDestination;
			Candidate.Score = ScoreDodgeCandidate(Agent, Direction, ProjectedDestination, BaseDirection);
			Candidate.bValid = true;

			DrawDodgeCandidateDebug(Agent, ProjectedDestination, FColor::Green);
			if (!bFoundValidCandidate || Candidate.Score > BestCandidate.Score)
			{
				BestCandidate = Candidate;
				bFoundValidCandidate = true;
			}
		}

		if (!bFoundValidCandidate)
		{
			return false;
		}

		DrawDodgeCandidateDebug(Agent, BestCandidate.Destination, FColor::Cyan);
		OutCandidate = BestCandidate;
		return true;
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

		FDodgeCandidate DodgeCandidate;
		if (!FindBestDodgeCandidate(*AgentPawn, *Controller, DodgeDistance, DodgeCandidate))
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeDangerAction[%s]: blocked, no valid dodge destination."), *GetNameSafe(AgentPawn));
			return;
		}

		UCharacterMovementComponent* CharacterMovement = AgentPawn->GetCharacterMovement();
		const float PreviousMaxSpeed = CharacterMovement ? CharacterMovement->MaxWalkSpeed : 0.0f;
		if (CharacterMovement && DodgeSpeed > 0.0f)
		{
			CharacterMovement->MaxWalkSpeed = DodgeSpeed;
		}

		AgentPawn->SetDodgeDirectionWorld(DodgeCandidate.Direction);
		DecisionComponent->AddCurrentState(TAG_State_Dodging.GetTag());

		Controller->StopMovement();

		if (UWorld* World = AgentPawn->GetWorld())
		{
			const TSharedRef<FTimerHandle> DodgeMovementTimerHandle = MakeShared<FTimerHandle>();
			const FVector DodgeStartLocation = AgentPawn->GetActorLocation();
			const FVector DodgeEndLocation = DodgeCandidate.Destination;
			const float DodgeStartTime = World->GetTimeSeconds();
			const TWeakObjectPtr<AAgent> MovementWeakAgent = WeakAgent;
			World->GetTimerManager().SetTimer(
				DodgeMovementTimerHandle.Get(),
				FTimerDelegate::CreateLambda([MovementWeakAgent, DodgeStartLocation, DodgeEndLocation, DodgeStartTime, MovementDuration]()
				{
					AAgent* MovingAgent = MovementWeakAgent.Get();
					UWorld* MovementWorld = MovingAgent ? MovingAgent->GetWorld() : nullptr;
					if (!MovingAgent || !MovementWorld)
					{
						return;
					}

					const float Alpha = MovementDuration > 0.0f
						? FMath::Clamp((MovementWorld->GetTimeSeconds() - DodgeStartTime) / MovementDuration, 0.0f, 1.0f)
						: 1.0f;
					const FVector NextLocation = FMath::Lerp(DodgeStartLocation, DodgeEndLocation, Alpha);
					FHitResult SweepHit;
					MovingAgent->SetActorLocation(NextLocation, true, &SweepHit);
				}),
				DodgeMovementTickRate,
				true);

			FTimerDelegate FinishDodgeDelegate;
			FinishDodgeDelegate.BindLambda([WeakDecisionComponent, WeakController, WeakAgent, DodgeMovementTimerHandle, DodgeEndLocation, PreviousMaxSpeed, DodgeSpeed]()
			{
				UDecisionComponent* DecisionComponent = WeakDecisionComponent.Get();
				AAgentAIController* Controller = WeakController.Get();
				AAgent* AgentPawn = WeakAgent.Get();
				if (AgentPawn)
				{
					if (UWorld* World = AgentPawn->GetWorld())
					{
						World->GetTimerManager().ClearTimer(DodgeMovementTimerHandle.Get());
					}
				}
				if (Controller)
				{
					Controller->StopMovement();
				}
				if (AgentPawn)
				{
					if (UCharacterMovementComponent* CharacterMovement = AgentPawn->GetCharacterMovement())
					{
						if (DodgeSpeed > 0.0f)
						{
							CharacterMovement->MaxWalkSpeed = PreviousMaxSpeed;
						}
					}
					AgentPawn->ClearDodgeDirection();
					FHitResult SweepHit;
					AgentPawn->SetActorLocation(DodgeEndLocation, true, &SweepHit);
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
