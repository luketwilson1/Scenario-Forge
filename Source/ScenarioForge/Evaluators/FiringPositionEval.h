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
};

/**
 * @brief Test evaluator that randomly scores positions with line of sight to the current target.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class SCENARIOFORGE_API URandomLineOfSightFiringPositionEval : public UFiringPositionEval
{
	GENERATED_BODY()

public:
	/** Returns a random score when the candidate has line of sight to the active tactical target. */
	virtual float EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const override;
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
