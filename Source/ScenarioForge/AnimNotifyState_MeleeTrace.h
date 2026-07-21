// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AnimNotifyState_MeleeTrace.h
 * @brief Declares the montage window that performs melee sphere traces between two bones.
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_MeleeTrace.generated.h"

/** Performs melee traces for the active GA_Melee throughout an animation window. */
UCLASS(meta = (DisplayName = "Melee Trace"))
class SCENARIOFORGE_API UAnimNotifyState_MeleeTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** Bone or socket at the beginning of the trace segment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace")
	FName StartBoneName = NAME_None;

	/** Bone or socket at the end of the trace segment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace")
	FName EndBoneName = NAME_None;

	/** Radius of the sphere swept between StartBoneName and EndBoneName. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float SphereRadius = 15.0f;

	/** Draws the swept melee volume in red while this notify is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace", meta = (DisplayName = "Draw Debug Trace"))
	bool bDrawDebugTrace = false;

	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	/** Forwards one trace request to the active per-agent melee ability instance. */
	void PerformTrace(USkeletalMeshComponent* MeshComp) const;

	/** Draws the sphere sweep as a capsule between the configured bones. */
	void DrawTraceDebug(USkeletalMeshComponent* MeshComp) const;
};
