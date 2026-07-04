// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FiringPositionEval.h
 * @brief Declares the base evaluator used to score tactical firing position candidates.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FiringPositionEval.generated.h"

class AFiringPosition;
class UTacticalPositioningComponent;

/**
 * @brief Base class for scoring firing position candidates.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCENARIOFORGE_API UFiringPositionEval : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief Evaluates a raw value for a firing position candidate.
	 *
	 * The owning configuration maps this raw value through its score curve and points value.
	 *
	 * @param PositioningComponent Tactical positioning component requesting the score.
	 * @param CandidatePosition Candidate firing position to score.
	 * @return Raw evaluator value consumed by the designer-authored score curve.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AI|Tactical Positioning")
	float EvaluateRawScore(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const;

	/** Evaluates a raw score with optional native debug visualization enabled by the evaluator config. */
	virtual float EvaluateRawScoreWithDebug(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition, bool bDebugDraw) const;
};

/**
 * @brief Pass/fail evaluator that accepts positions with line of sight to the current target.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "FPE_LineOfSight"))
class SCENARIOFORGE_API UFPE_LineOfSight : public UFiringPositionEval
{
	GENERATED_BODY()

public:
	/** Returns 1 when the candidate has line of sight to the active tactical target, otherwise 0. */
	virtual float EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const override;
};

/**
 * @brief Pass/fail evaluator that accepts positions where the current weapon can reach the target.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "FPE_WeaponRange"))
class SCENARIOFORGE_API UFPE_WeaponRange : public UFiringPositionEval
{
	GENERATED_BODY()

public:
	/** Returns 1 when the active tactical target is inside the current weapon's firing range, otherwise 0. */
	virtual float EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const override;
};

/**
 * @brief Pass/fail evaluator that accepts positions reachable through the navmesh.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "FPE_Reachable"))
class SCENARIOFORGE_API UFPE_Reachable : public UFiringPositionEval
{
	GENERATED_BODY()

public:
	/** Returns 1 when the controlled pawn can path to the candidate position, otherwise 0. */
	virtual float EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const override;
};

/**
 * @brief Pass/fail evaluator that accepts positions with line of sight to a remembered enemy location.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "FPE_LastKnownLOS"))
class SCENARIOFORGE_API UFPE_LastKnownLOS : public UFiringPositionEval
{
	GENERATED_BODY()

public:
	/** Returns 1 when the candidate has line of sight to the most recent remembered enemy location, otherwise 0. */
	virtual float EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const override;

	/** Draws the remembered-location trace when this evaluator's debug toggle is enabled. */
	virtual float EvaluateRawScoreWithDebug(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition, bool bDebugDraw) const override;
};

/**
 * @brief Idle evaluator that randomly scores firing positions for patrol-style movement.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "RandomPatrol"))
class SCENARIOFORGE_API URandomPatrolFiringPositionEval : public UFiringPositionEval
{
	GENERATED_BODY()

public:
	/** Returns a random score for any valid candidate. */
	virtual float EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const override;
};
