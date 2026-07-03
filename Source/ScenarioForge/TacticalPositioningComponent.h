// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file TacticalPositioningComponent.h
 * @brief Declares the component that owns tactical movement intent and position selection.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TacticalPositioningComponent.generated.h"

class AFiringPosition;
class ASquad;

/**
 * @brief High-level tactical posture used when evaluating movement destinations.
 */
UENUM(BlueprintType)
enum class ETacticalMovementMode : uint8
{
	/** No tactical movement intent is active. */
	None UMETA(DisplayName = "None"),

	/** Non-combat tactical movement, such as patrol or idle repositioning. */
	Idle = 1 UMETA(DisplayName = "Idle"),

	/** Movement used after losing sight of a previously seen target. */
	Search = 2 UMETA(DisplayName = "Search"),

	/** Prefer defensive positions and remain behind cover for cover-specific timing. */
	Cover = 3 UMETA(DisplayName = "Cover"),

	/** Prefer balanced combat positions with cover, line of fire, and good range. */
	Combat = 4 UMETA(DisplayName = "Combat"),

	/** Leave cover and move to a position chosen by uncover-specific evaluators. */
	Uncover = 5 UMETA(DisplayName = "Uncover")
};

/**
 * @brief Maintains tactical movement intent separately from physical character movement.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class SCENARIOFORGE_API UTacticalPositioningComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Initializes component defaults. */
	UTacticalPositioningComponent();

	/**
	 * @brief Sets the movement evaluation mode this component should use.
	 *
	 * @param NewMovementMode Tactical movement mode to activate.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Tactical Positioning")
	void SetMovementMode(ETacticalMovementMode NewMovementMode);

	/**
	 * @brief Gets the currently active tactical movement mode.
	 *
	 * @return Active tactical movement mode.
	 */
	UFUNCTION(BlueprintPure, Category = "AI|Tactical Positioning")
	ETacticalMovementMode GetMovementMode() const;

	/**
	 * @brief Sets the actor this component should position around.
	 *
	 * @param NewTargetActor Actor used as the tactical positioning target.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Tactical Positioning")
	void SetTargetActor(AActor* NewTargetActor);

	/**
	 * @brief Gets the current tactical target actor.
	 *
	 * @return Target actor, or nullptr when no target is set.
	 */
	UFUNCTION(BlueprintPure, Category = "AI|Tactical Positioning")
	AActor* GetTargetActor() const;

	/** Clears the current tactical target actor. */
	UFUNCTION(BlueprintCallable, Category = "AI|Tactical Positioning")
	void ClearTargetActor();

	/**
	 * @brief Sets the currently selected destination.
	 *
	 * @param NewDestination World-space destination selected by tactical evaluation.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Tactical Positioning")
	void SetDesiredDestination(const FVector& NewDestination);

	/**
	 * @brief Gets the currently selected destination.
	 *
	 * @param OutDestination Destination value when one is active.
	 * @return True when a desired destination is available.
	 */
	UFUNCTION(BlueprintPure, Category = "AI|Tactical Positioning")
	bool GetDesiredDestination(FVector& OutDestination) const;

	/** Clears the currently selected destination. */
	UFUNCTION(BlueprintCallable, Category = "AI|Tactical Positioning")
	void ClearDesiredDestination();

protected:
	/**
	 * @brief Ticks tactical positioning state for future evaluation updates.
	 *
	 * @param DeltaTime Seconds elapsed since the previous tick.
	 * @param TickType Tick type supplied by Unreal.
	 * @param ThisTickFunction Tick function metadata supplied by Unreal.
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Scores available squad-zone firing positions and moves toward the best candidate when one exists. */
	bool EvaluateAndMoveToBestFiringPosition();

	/** Returns whether the controlled pawn has reached DesiredDestination. */
	bool HasReachedDesiredDestination() const;

	/** Finds the squad that owns this component's possessed agent. */
	ASquad* FindOwningSquad() const;

	/** Scores one candidate firing position using the active movement mode's evaluator set. */
	float ScoreFiringPosition(const AFiringPosition* CandidatePosition) const;

	/** Schedules the next reposition after arriving at a movement-mode destination. */
	void ScheduleNextReposition();

	/** Schedules the next reposition attempt immediately. */
	void ScheduleImmediateReposition();

	/** Gets the resolved acceptance radius used for reposition move requests. */
	float GetRepositionAcceptanceRadius() const;

protected:
	/** Active tactical movement evaluation mode. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Tactical Positioning")
	ETacticalMovementMode MovementMode = ETacticalMovementMode::None;

	/** Actor used as the focus for tactical position scoring. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Tactical Positioning")
	TObjectPtr<AActor> TargetActor;

	/** Last destination selected by the tactical positioning layer. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Tactical Positioning")
	FVector DesiredDestination = FVector::ZeroVector;

	/** True when DesiredDestination contains an active movement objective. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Tactical Positioning")
	bool bHasDesiredDestination = false;

	/** True while waiting for path following to reach DesiredDestination. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Tactical Positioning")
	bool bWaitingForMoveCompletion = false;

	/** World time when the next reposition evaluation should run. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Tactical Positioning")
	float NextRepositionTime = 0.0f;

	/** True only after reaching cover and waiting for the hide-behind-cover timer. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Tactical Positioning")
	bool bCoverHoldTimerActive = false;
};
