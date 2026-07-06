// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GrenadeThrowFunctionLibrary.cpp
 * @brief Implements shared grenade trajectory helpers for AI grenade decisions and actions.
 */

#include "GrenadeThrowFunctionLibrary.h"

#include "Agent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"

namespace
{
	FVector2D GetTrajectoryArcRatioRange(const EGrenadeTrajectoryType TrajectoryType)
	{
		switch (TrajectoryType)
		{
		case EGrenadeTrajectoryType::Lob:
			return FVector2D(0.30f, 0.60f);
		case EGrenadeTrajectoryType::Toss:
		case EGrenadeTrajectoryType::Bounce:
		default:
			return FVector2D(0.15f, 0.30f);
		}
	}

	bool IsSpeedInsideMaxVelocity(const float Speed, const float MaxVelocity)
	{
		return MaxVelocity > 0.0f && Speed <= MaxVelocity;
	}

	float GetMaxVelocityMissDistance(const float Speed, const float MaxVelocity)
	{
		if (MaxVelocity <= 0.0f)
		{
			return Speed;
		}

		return Speed > MaxVelocity ? Speed - MaxVelocity : 0.0f;
	}

	float GetIdealVelocityMissDistance(const float Speed, const float IdealVelocity)
	{
		return IdealVelocity > 0.0f ? FMath::Abs(Speed - IdealVelocity) : 0.0f;
	}

	bool CalculateLaunchVelocityForArcRatio(
		const UWorld& World,
		const FVector& LaunchLocation,
		const FVector& LandingLocation,
		const FGrenadeProperties& GrenadeProperties,
		const float ArcRatio,
		FGrenadeThrowSolution& OutSolution)
	{
		const FVector ToLanding = LandingLocation - LaunchLocation;
		const FVector HorizontalOffset(ToLanding.X, ToLanding.Y, 0.0f);
		const float HorizontalRange = HorizontalOffset.Size();
		if (HorizontalRange <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		const float Gravity = FMath::Abs(World.GetGravityZ());
		if (Gravity <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		const FVector HorizontalDirection = HorizontalOffset / HorizontalRange;
		const float ArcHeight = FMath::Max(1.0f, HorizontalRange * ArcRatio);
		const float ApexZ = FMath::Max(LaunchLocation.Z, LandingLocation.Z) + ArcHeight;
		const float HeightAboveLaunch = ApexZ - LaunchLocation.Z;
		const float HeightAboveLanding = ApexZ - LandingLocation.Z;

		if (HeightAboveLaunch <= 0.0f || HeightAboveLanding <= 0.0f)
		{
			return false;
		}

		const float VerticalSpeed = FMath::Sqrt(2.0f * Gravity * HeightAboveLaunch);
		const float TimeUp = VerticalSpeed / Gravity;
		const float TimeDown = FMath::Sqrt((2.0f * HeightAboveLanding) / Gravity);
		const float FlightTime = TimeUp + TimeDown;
		if (FlightTime <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		const FVector HorizontalVelocity = HorizontalDirection * (HorizontalRange / FlightTime);
		const FVector LaunchVelocity = HorizontalVelocity + FVector::UpVector * VerticalSpeed;

		OutSolution.LaunchLocation = LaunchLocation;
		OutSolution.LandingLocation = LandingLocation;
		OutSolution.LaunchVelocity = LaunchVelocity;
		OutSolution.ArcHeight = ArcHeight;
		OutSolution.FlightTime = FlightTime;
		OutSolution.bVelocityInRange = IsSpeedInsideMaxVelocity(LaunchVelocity.Size(), GrenadeProperties.GrenadeVelocity);
		OutSolution.bTrajectoryBlocked = false;
		OutSolution.BlockingHit = FHitResult();
		return true;
	}

	void UpdateTrajectoryBlockState(const AAgent& ThrowingAgent, FGrenadeThrowSolution& Solution)
	{
		UWorld* World = ThrowingAgent.GetWorld();
		if (!World)
		{
			return;
		}

		constexpr float ProjectileTraceRadius = 12.0f;
		constexpr float LandingHitTolerance = 100.0f;
		constexpr float SimulationPaddingSeconds = 0.1f;

		FPredictProjectilePathParams PathParams;
		PathParams.StartLocation = Solution.LaunchLocation;
		PathParams.LaunchVelocity = Solution.LaunchVelocity;
		PathParams.MaxSimTime = Solution.FlightTime + SimulationPaddingSeconds;
		PathParams.ProjectileRadius = ProjectileTraceRadius;
		PathParams.TraceChannel = ECC_Visibility;
		PathParams.bTraceWithCollision = true;
		PathParams.bTraceWithChannel = true;
		PathParams.ActorsToIgnore.Add(const_cast<AAgent*>(&ThrowingAgent));

		FPredictProjectilePathResult PathResult;
		UGameplayStatics::PredictProjectilePath(World, PathParams, PathResult);

		Solution.bTrajectoryBlocked = false;
		Solution.BlockingHit = FHitResult();

		if (!PathResult.HitResult.bBlockingHit)
		{
			return;
		}

		Solution.BlockingHit = PathResult.HitResult;
		const float HitHorizontalDistanceToLanding = FVector::Dist2D(PathResult.HitResult.ImpactPoint, Solution.LandingLocation);
		Solution.bTrajectoryBlocked = HitHorizontalDistanceToLanding > LandingHitTolerance;
	}
}

bool UGrenadeThrowFunctionLibrary::CalculateLaunchVelocity(
	UWorld* World,
	const FVector& LaunchLocation,
	const FVector& LandingLocation,
	const FGrenadeProperties& GrenadeProperties,
	FGrenadeThrowSolution& OutSolution)
{
	if (!World)
	{
		return false;
	}

	const FVector2D ArcRatioRange = GetTrajectoryArcRatioRange(GrenadeProperties.TrajectoryType);
	const float MinArcRatio = FMath::Min(ArcRatioRange.X, ArcRatioRange.Y);
	const float MaxArcRatio = FMath::Max(ArcRatioRange.X, ArcRatioRange.Y);
	constexpr float ArcRatioStep = 0.05f;

	bool bFoundCandidate = false;
	float BestScore = TNumericLimits<float>::Max();
	FGrenadeThrowSolution BestSolution;

	for (float ArcRatio = MinArcRatio; ArcRatio <= MaxArcRatio + KINDA_SMALL_NUMBER; ArcRatio += ArcRatioStep)
	{
		FGrenadeThrowSolution CandidateSolution;
		if (!CalculateLaunchVelocityForArcRatio(*World, LaunchLocation, LandingLocation, GrenadeProperties, ArcRatio, CandidateSolution))
		{
			continue;
		}

		const float Speed = CandidateSolution.LaunchVelocity.Size();
		const float Score = GetMaxVelocityMissDistance(Speed, GrenadeProperties.GrenadeVelocity) * 1000.0f
			+ GetIdealVelocityMissDistance(Speed, GrenadeProperties.GrenadeIdealVelocity);

		if (!bFoundCandidate || Score < BestScore)
		{
			bFoundCandidate = true;
			BestScore = Score;
			BestSolution = CandidateSolution;
		}
	}

	if (!bFoundCandidate)
	{
		return false;
	}

	OutSolution = BestSolution;
	return true;
}

bool UGrenadeThrowFunctionLibrary::BuildThrowSolutionForAgent(
	const AAgent* ThrowingAgent,
	const FVector& LandingLocation,
	const FGrenadeProperties& GrenadeProperties,
	FGrenadeThrowSolution& OutSolution)
{
	if (!ThrowingAgent)
	{
		return false;
	}

	UWorld* World = ThrowingAgent->GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector LaunchLocation = ThrowingAgent->GetActorLocation();
	const FVector2D ArcRatioRange = GetTrajectoryArcRatioRange(GrenadeProperties.TrajectoryType);
	const float MinArcRatio = FMath::Min(ArcRatioRange.X, ArcRatioRange.Y);
	const float MaxArcRatio = FMath::Max(ArcRatioRange.X, ArcRatioRange.Y);
	constexpr float ArcRatioStep = 0.05f;

	bool bFoundCandidate = false;
	float BestFailedScore = TNumericLimits<float>::Max();
	FGrenadeThrowSolution BestFailedSolution;
	bool bFoundValidSolution = false;
	float BestValidScore = TNumericLimits<float>::Max();
	FGrenadeThrowSolution BestValidSolution;

	for (float ArcRatio = MinArcRatio; ArcRatio <= MaxArcRatio + KINDA_SMALL_NUMBER; ArcRatio += ArcRatioStep)
	{
		FGrenadeThrowSolution CandidateSolution;
		if (!CalculateLaunchVelocityForArcRatio(*World, LaunchLocation, LandingLocation, GrenadeProperties, ArcRatio, CandidateSolution))
		{
			continue;
		}

		UpdateTrajectoryBlockState(*ThrowingAgent, CandidateSolution);
		const bool bValidThrow = CandidateSolution.bVelocityInRange && !CandidateSolution.bTrajectoryBlocked;
		if (bValidThrow)
		{
			const float Speed = CandidateSolution.LaunchVelocity.Size();
			const float IdealScore = GetIdealVelocityMissDistance(Speed, GrenadeProperties.GrenadeIdealVelocity);
			if (!bFoundValidSolution || IdealScore < BestValidScore)
			{
				bFoundValidSolution = true;
				BestValidScore = IdealScore;
				BestValidSolution = CandidateSolution;
			}

			if (GrenadeProperties.GrenadeIdealVelocity <= 0.0f || Speed >= GrenadeProperties.GrenadeIdealVelocity)
			{
				break;
			}
		}

		const float Speed = CandidateSolution.LaunchVelocity.Size();
		const float FailedScore = GetMaxVelocityMissDistance(Speed, GrenadeProperties.GrenadeVelocity) * 1000.0f
			+ (CandidateSolution.bTrajectoryBlocked ? 100.0f : 0.0f)
			+ GetIdealVelocityMissDistance(Speed, GrenadeProperties.GrenadeIdealVelocity);

		if (!bFoundCandidate || FailedScore < BestFailedScore)
		{
			bFoundCandidate = true;
			BestFailedScore = FailedScore;
			BestFailedSolution = CandidateSolution;
		}
	}

	if (bFoundValidSolution)
	{
		OutSolution = BestValidSolution;
		return true;
	}

	if (!bFoundCandidate)
	{
		return false;
	}

	OutSolution = BestFailedSolution;
	return true;
}

void UGrenadeThrowFunctionLibrary::DrawDebugTrajectory(UWorld* World, const FGrenadeThrowSolution& Solution, const float Duration)
{
	if (!World)
	{
		return;
	}

	const FColor TrajectoryColor = !Solution.bVelocityInRange ? FColor::Red : Solution.bTrajectoryBlocked ? FColor::Yellow : FColor::Green;
	const FVector GravityAcceleration(0.0f, 0.0f, World->GetGravityZ());
	constexpr int32 TrajectorySegments = 24;

	FVector PreviousPoint = Solution.LaunchLocation;
	FVector ApexLocation = Solution.LaunchLocation;
	for (int32 SegmentIndex = 1; SegmentIndex <= TrajectorySegments; ++SegmentIndex)
	{
		const float Time = Solution.FlightTime * (static_cast<float>(SegmentIndex) / static_cast<float>(TrajectorySegments));
		const FVector CurrentPoint = Solution.LaunchLocation + Solution.LaunchVelocity * Time + 0.5f * GravityAcceleration * FMath::Square(Time);

		DrawDebugLine(World, PreviousPoint, CurrentPoint, TrajectoryColor, false, Duration, 0, 3.0f);
		if (CurrentPoint.Z > ApexLocation.Z)
		{
			ApexLocation = CurrentPoint;
		}

		PreviousPoint = CurrentPoint;
	}

	DrawDebugSphere(World, Solution.LaunchLocation, 25.0f, 12, FColor::Blue, false, Duration, 0, 2.0f);
	DrawDebugSphere(World, ApexLocation, 35.0f, 12, FColor::Cyan, false, Duration, 0, 2.0f);
	DrawDebugSphere(World, Solution.LandingLocation, 50.0f, 16, FColor::Orange, false, Duration, 0, 3.0f);
	if (Solution.bTrajectoryBlocked)
	{
		DrawDebugSphere(World, Solution.BlockingHit.ImpactPoint, 40.0f, 12, FColor::Yellow, false, Duration, 0, 4.0f);
	}
}
