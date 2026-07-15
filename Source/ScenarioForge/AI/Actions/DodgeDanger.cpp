// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file DodgeDanger.cpp
 * @brief Implements the GOAP danger dodge action.
 */

#include "DodgeDanger.h"

#include "AIController.h"
#include "Agent.h"
#include "AgentAIController.h"
#include "AgentCustomization.h"
#include "Planner.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "GameplayAbilitySystem/GameplayEffects/GE_BurstSeparation.h"
#include "NavigationSystem.h"
#include "PawnCustomization.h"
#include "ScenarioForgeGameplayTags.h"
#include "TimerManager.h"

namespace
{
	constexpr float DodgeStateFallbackDuration = 0.75f;
	constexpr float MinimumDodgeStateDuration = 0.1f;
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

	bool FindBestDodgeCandidate(const AAgent& Agent, const AAgentAIController& AgentAIController, float DodgeDistance, FDodgeCandidate& OutCandidate)
	{
		const FVector BaseDirection = ResolveBaseDodgeDirection(Agent, AgentAIController);
		if (BaseDirection.IsNearlyZero())
		{
			return false;
		}

		static constexpr float CandidateAngles[] = { 0.0f, 90.0f, -90.0f };
		bool bFoundValidCandidate = false;
		FDodgeCandidate BestCandidate;

		for (const float CandidateAngle : CandidateAngles)
		{
			const FVector Direction = RotateDirection2D(BaseDirection, CandidateAngle);
			const FVector DesiredDestination = Agent.GetActorLocation() + Direction * DodgeDistance;
			FVector ProjectedDestination = FVector::ZeroVector;
			if (!ProjectDodgeDestination(Agent, DesiredDestination, ProjectedDestination))
			{
				continue;
			}

			FHitResult SweepHit;
			if (!CapsuleSweepToDestination(Agent, ProjectedDestination, SweepHit))
			{
				continue;
			}

			FDodgeCandidate Candidate;
			Candidate.Direction = Direction;
			Candidate.Destination = ProjectedDestination;
			Candidate.Score = ScoreDodgeCandidate(Agent, Direction, ProjectedDestination, BaseDirection);
			Candidate.bValid = true;

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

		OutCandidate = BestCandidate;
		return true;
	}

	void ApplyDodgeCooldown(AAgent& Agent, float CooldownDuration)
	{
		if (CooldownDuration <= 0.0f)
		{
			return;
		}

		UAbilitySystemComponent* AbilitySystemComponent = Agent.GetAbilitySystemComponent();
		if (!AbilitySystemComponent)
		{
			return;
		}

		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(&Agent);

		FGameplayEffectSpecHandle CooldownSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
			UGE_BurstSeparation::StaticClass(),
			1.0f,
			EffectContext);

		if (!CooldownSpecHandle.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeDangerAction[%s]: failed to create dodge cooldown effect."), *GetNameSafe(&Agent));
			return;
		}

		CooldownSpecHandle.Data->SetDuration(CooldownDuration, true);
		CooldownSpecHandle.Data->DynamicGrantedTags.AddTag(TAG_Cooldown_AI_DodgeDanger.GetTag());
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*CooldownSpecHandle.Data.Get());
	}

	UAnimMontage* SelectDodgeMontage(const AAgent& Agent)
	{
		const UPawnCustomization* PawnCustomization = Agent.GetResolvedPawnCustomization();
		if (!PawnCustomization)
		{
			return nullptr;
		}

		const float LocalRight = Agent.GetDodgeDirectionLocal().Y;
		if (LocalRight < -0.33f && PawnCustomization->LeftDodgeMontage)
		{
			return PawnCustomization->LeftDodgeMontage;
		}

		if (LocalRight > 0.33f && PawnCustomization->RightDodgeMontage)
		{
			return PawnCustomization->RightDodgeMontage;
		}

		return PawnCustomization->ForwardDodgeMontage;
	}

	void FinishDodge(
		const TWeakObjectPtr<UPlanner>& Planner,
		const TWeakObjectPtr<AAgentAIController>& Controller,
		const TWeakObjectPtr<AAgent>& Agent)
	{
		if (AAgentAIController* ResolvedController = Controller.Get())
		{
			ResolvedController->StopMovement();
		}

		if (AAgent* ResolvedAgent = Agent.Get())
		{
			ResolvedAgent->ClearDodgeDirection();
		}

		if (UPlanner* ResolvedPlanner = Planner.Get())
		{
			ResolvedPlanner->RemoveCurrentState(TAG_State_Dodging.GetTag());
		}
	}

	bool TryPlayDodgeMontage(
		AAgent& Agent,
		const TWeakObjectPtr<UPlanner>& Planner,
		const TWeakObjectPtr<AAgentAIController>& Controller,
		const TWeakObjectPtr<AAgent>& WeakAgent)
	{
		UAnimMontage* DodgeMontage = SelectDodgeMontage(Agent);
		USkeletalMeshComponent* MeshComponent = Agent.GetMesh();
		UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
		if (!DodgeMontage || !AnimInstance)
		{
			return false;
		}

		const float MontageDuration = AnimInstance->Montage_Play(DodgeMontage);
		if (MontageDuration <= 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeDangerAction[%s]: failed to play dodge montage %s."), *GetNameSafe(&Agent), *GetNameSafe(DodgeMontage));
			return false;
		}

		FOnMontageEnded EndDelegate;
		EndDelegate.BindLambda([Planner, Controller, WeakAgent](UAnimMontage* Montage, bool bInterrupted)
		{
			(void)Montage;
			(void)bInterrupted;

			FinishDodge(Planner, Controller, WeakAgent);
		});
		AnimInstance->Montage_SetEndDelegate(EndDelegate, DodgeMontage);

		return true;
	}
}

/**
 * @brief Configures the dodge action's planning preconditions and effects.
 */
UDodgeDanger::UDodgeDanger()
{
	TruePreconditions.AddTag(TAG_State_Danger_Grenade.GetTag());
	FalsePreconditions.AddTag(TAG_Cooldown_AI_DodgeDanger.GetTag());
	AddedEffects.AddTag(TAG_State_SafeFromDanger.GetTag());
}

/**
 * @brief Schedules a dodge using the owning agent's resolved dodge settings.
 *
 * @param Planner Planner executing the dodge action.
 * @return Running when the dodge is scheduled, Failed when it cannot start, or Invalid for a bad execution context.
 */
EActionResult UDodgeDanger::Execute(UPlanner* Planner)
{
	if (!Planner)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeDangerAction: blocked, missing planner."));
		return EActionResult::Invalid;
	}

	AAgentAIController* AgentAIController = Cast<AAgentAIController>(Planner->GetOwner());
	AAgent* OwningAgent = AgentAIController ? Cast<AAgent>(AgentAIController->GetPawn()) : nullptr;
	const UAgentCustomization* AgentCustomization = OwningAgent ? OwningAgent->GetAgentCustomization() : nullptr;
	if (!AgentAIController || !OwningAgent || !AgentCustomization)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeDangerAction: blocked, controller=%s agent=%s customization=%s."),
			AgentAIController ? TEXT("true") : TEXT("false"),
			OwningAgent ? TEXT("true") : TEXT("false"),
			AgentCustomization ? TEXT("true") : TEXT("false"));
		return EActionResult::Invalid;
	}

	const FDodgeProperties& DodgeProperties = AgentCustomization->GetResolvedDodgeProperties();
	const float DodgeDistance = FMath::Max(0.0f, DodgeProperties.Distance);
	const float DodgeSpeed = FMath::Max(0.0f, DodgeProperties.Speed);
	const float ReactionDelay = FMath::Max(0.0f, DodgeProperties.ReactionDelay);
	const float DodgeCooldown = FMath::Max(0.0f, DodgeProperties.Cooldown);
	if (DodgeDistance <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeDangerAction[%s]: blocked, dodge distance is 0."), *GetNameSafe(OwningAgent));
		return EActionResult::Failed;
	}

	UAbilitySystemComponent* AbilitySystemComponent = OwningAgent->GetAbilitySystemComponent();
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(TAG_Cooldown_AI_DodgeDanger.GetTag()))
	{
		return EActionResult::Failed;
	}

	UWorld* World = OwningAgent->GetWorld();
	if (!World)
	{
		return EActionResult::Invalid;
	}

	const TWeakObjectPtr<UPlanner> WeakPlanner = Planner;
	const TWeakObjectPtr<AAgentAIController> WeakController = AgentAIController;
	const TWeakObjectPtr<AAgent> WeakAgent = OwningAgent;
	const float DodgeStateDuration = DodgeSpeed > 0.0f
		? FMath::Max(MinimumDodgeStateDuration, DodgeDistance / DodgeSpeed)
		: DodgeStateFallbackDuration;

	FTimerDelegate StartDodgeDelegate;
	StartDodgeDelegate.BindLambda([WeakPlanner, WeakController, WeakAgent, DodgeDistance, DodgeStateDuration, DodgeCooldown]()
	{
		UPlanner* Planner = WeakPlanner.Get();
		AAgentAIController* Controller = WeakController.Get();
		AAgent* AgentPawn = WeakAgent.Get();
		if (!Planner || !Controller || !AgentPawn)
		{
			return;
		}

		FDodgeCandidate DodgeCandidate;
		if (!FindBestDodgeCandidate(*AgentPawn, *Controller, DodgeDistance, DodgeCandidate))
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeDangerAction[%s]: blocked, no valid dodge destination."), *GetNameSafe(AgentPawn));
			return;
		}

		AgentPawn->SetDodgeDirectionWorld(DodgeCandidate.Direction);
		Planner->AddCurrentState(TAG_State_Dodging.GetTag());
		ApplyDodgeCooldown(*AgentPawn, DodgeCooldown);

		Controller->StopMovement();

		if (TryPlayDodgeMontage(*AgentPawn, WeakPlanner, WeakController, WeakAgent))
		{
			return;
		}

		if (UWorld* World = AgentPawn->GetWorld())
		{
			FTimerDelegate FinishDodgeDelegate;
			FinishDodgeDelegate.BindLambda([WeakPlanner, WeakController, WeakAgent]()
			{
				FinishDodge(WeakPlanner, WeakController, WeakAgent);
			});

			FTimerHandle FinishDodgeTimerHandle;
			World->GetTimerManager().SetTimer(FinishDodgeTimerHandle, FinishDodgeDelegate, DodgeStateDuration, false);
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

	return EActionResult::Running;
}
