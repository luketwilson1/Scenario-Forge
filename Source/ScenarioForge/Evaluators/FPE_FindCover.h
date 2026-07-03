// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FPE_FindCover.h
 * @brief Declares the firing-position evaluator that scores cover candidates.
 */

#pragma once

#include "CoreMinimal.h"
#include "FiringPositionEval.h"
#include "FPE_FindCover.generated.h"

/**
 * @brief Scores authored firing positions by cover quality.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "FPE_FindCover"))
class SCENARIOFORGE_API UFPE_FindCover : public UFiringPositionEval
{
	GENERATED_BODY()

public:
	/** Score returned for partial-cover firing positions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
	float PartialCoverScore = 0.65f;

	/** Score returned for closed-cover firing positions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
	float ClosedCoverScore = 1.0f;

	/** Score returned for wall-lean firing positions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cover", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
	float WallLeanScore = 0.9f;

	/** If true, rejects candidates that are directly visible from the current tactical target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat")
	bool bRequireBlockedLineOfSightFromCurrentTarget = false;

	/** Height above the firing position used when checking whether the current target can see it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float CoverCheckEyeHeight = 100.0f;

	/** Returns a score based on generated cover flags, optionally filtered by current target visibility. */
	virtual float EvaluateRawScore_Implementation(const UTacticalPositioningComponent* PositioningComponent, const AFiringPosition* CandidatePosition) const override;
};
