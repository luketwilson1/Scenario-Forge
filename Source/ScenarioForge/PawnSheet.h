// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file PawnSheet.h
 * @brief Declares shared pawn presentation data used by agents and playable characters.
 */

#pragma once

#include "CoreMinimal.h"
#include "SheetTypes.h"
#include "Engine/DataAsset.h"
#include "PawnSheet.generated.h"

class UAnimMontage;

/**
 * @brief Stores reusable pawn appearance data such as skeletal mesh, animation blueprint, and material overrides.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API UPawnSheet : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Visual presentation used by the pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FAppearance Appearance;

	/** Montage played when this pawn throws a grenade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Grenade")
	TObjectPtr<UAnimMontage> ThrowGrenadeMontage;

	/** Montage played when this pawn performs a melee attack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Melee")
	TObjectPtr<UAnimMontage> MeleeMontage;

	/** Montage played when this pawn dodges forward away from danger. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Dodge")
	TObjectPtr<UAnimMontage> ForwardDodgeMontage;

	/** Montage played when this pawn dodges left away from danger. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Dodge")
	TObjectPtr<UAnimMontage> LeftDodgeMontage;

	/** Montage played when this pawn dodges right away from danger. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Dodge")
	TObjectPtr<UAnimMontage> RightDodgeMontage;

	/** Mesh socket used to attach the equipped weapon to this pawn's right hand. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sockets", meta = (DisplayName = "Right Hand Socket"))
	FName RightHandSocketName = TEXT("RightHand");

	/** Mesh socket used by future off-hand weapon or IK attachment logic. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sockets", meta = (DisplayName = "Left Hand Socket"))
	FName LeftHandSocketName = TEXT("LeftHand");

	/** Mesh socket used as the release point for thrown grenades. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sockets", meta = (DisplayName = "Grenade Release Socket"))
	FName GrenadeReleaseSocketName = TEXT("grenade_release_r");
};
