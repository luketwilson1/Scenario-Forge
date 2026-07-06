// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file GrenadeThrowFunctionLibrary.h
 * @brief Declares shared grenade trajectory helpers for AI grenade decisions and actions.
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AgentCustomization.h"
#include "GrenadeThrowFunctionLibrary.generated.h"

class AAgent;
class UWorld;

/**
 * @brief Result of solving a grenade throw trajectory.
 */
USTRUCT(BlueprintType)
struct SCENARIOFORGE_API FGrenadeThrowSolution
{
	GENERATED_BODY()

	/** World-space point where the grenade should be spawned from. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	FVector LaunchLocation = FVector::ZeroVector;

	/** World-space point where the grenade should land. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	FVector LandingLocation = FVector::ZeroVector;

	/** World-space launch velocity in cm/s. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	FVector LaunchVelocity = FVector::ZeroVector;

	/** Height above the higher endpoint used to solve the arc. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	float ArcHeight = 0.0f;

	/** Predicted flight time in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	float FlightTime = 0.0f;

	/** Whether LaunchVelocity is at or below the configured max grenade velocity. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	bool bVelocityInRange = false;

	/** Whether the predicted projectile path hits blocking geometry before the landing area. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	bool bTrajectoryBlocked = false;

	/** First blocking hit found while predicting the projectile path. */
	UPROPERTY(BlueprintReadOnly, Category = "Grenade")
	FHitResult BlockingHit;
};

/**
 * @brief Shared grenade throw calculations used by AI decision checks and throw actions.
 */
UCLASS()
class SCENARIOFORGE_API UGrenadeThrowFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief Solves the best launch velocity for a specific grenade landing point.
	 *
	 * @param World World whose gravity is used for the trajectory solve.
	 * @param LaunchLocation World-space start location for the grenade.
	 * @param LandingLocation World-space desired landing point.
	 * @param GrenadeProperties Agent grenade behavior settings.
	 * @param OutSolution Calculated throw solution.
	 * @return True when a projectile arc can be solved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	static bool CalculateLaunchVelocity(
		UWorld* World,
		const FVector& LaunchLocation,
		const FVector& LandingLocation,
		const FGrenadeProperties& GrenadeProperties,
		FGrenadeThrowSolution& OutSolution);

	/**
	 * @brief Solves a launch velocity and traces the predicted path for blocking geometry.
	 *
	 * @param ThrowingAgent Agent that will throw the grenade and should be ignored by the trajectory trace.
	 * @param LandingLocation World-space desired landing point.
	 * @param GrenadeProperties Agent grenade behavior settings.
	 * @param OutSolution Calculated throw solution.
	 * @return True when a projectile arc can be solved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	static bool BuildThrowSolutionForAgent(
		const AAgent* ThrowingAgent,
		const FVector& LandingLocation,
		const FGrenadeProperties& GrenadeProperties,
		FGrenadeThrowSolution& OutSolution);

	/**
	 * @brief Draws the solved trajectory and key points for debugging.
	 *
	 * @param World World to draw in.
	 * @param Solution Solved grenade throw data.
	 * @param Duration Debug draw duration in seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grenade|Debug")
	static void DrawDebugTrajectory(UWorld* World, const FGrenadeThrowSolution& Solution, float Duration = 3.0f);
};
