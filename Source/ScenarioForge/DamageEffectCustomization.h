// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file DamageEffectCustomization.h
 * @brief Declares the data asset used to describe damage effect values.
 */

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "DamageEffectCustomization.generated.h"

/**
 * @brief Falloff modes used to map distance from damage origin to damage amount.
 */
UENUM(BlueprintType)
enum class EDamageFalloffType : uint8
{
	None UMETA(DisplayName = "None"),
	Binary UMETA(DisplayName = "Binary"),
	Linear UMETA(DisplayName = "Linear"),
	Exponential UMETA(DisplayName = "Exponential"),
	CustomCurve UMETA(DisplayName = "Custom Curve")
};

/**
 * @brief Reach-style damage effect sheet referenced by projectiles, explosives, and abilities.
 */
UCLASS(BlueprintType)
class SCENARIOFORGE_API UDamageEffectCustomization : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Inner radius, in centimeters, where this damage effect applies full upper-bound damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Effect", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float InnerRadius = 0.0f;

	/** Outer radius, in centimeters, where this damage effect reaches lower-bound damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Effect", meta = (Units = "Centimeters", ClampMin = "0.0", UIMin = "0.0"))
	float OuterRadius = 0.0f;

	/** Minimum damage this effect can apply. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamageLowerBound = 0.0f;

	/** Maximum damage this effect can apply. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamageUpperBound = 0.0f;

	/** Falloff mode used between inner and outer radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Effect")
	EDamageFalloffType FalloffType = EDamageFalloffType::Linear;

	/** Optional curve that maps normalized falloff distance 0-1 to a damage alpha. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Effect", meta = (EditCondition = "FalloffType == EDamageFalloffType::CustomCurve"))
	TObjectPtr<UCurveFloat> FalloffCurve;

	/**
	 * @brief Gets a damage value inside this sheet's configured damage range.
	 *
	 * @return Damage amount selected from lower and upper bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "Damage Effect")
	float GetDamageAmount() const
	{
		const float MinDamage = FMath::Min(DamageLowerBound, DamageUpperBound);
		const float MaxDamage = FMath::Max(DamageLowerBound, DamageUpperBound);
		return FMath::IsNearlyEqual(MinDamage, MaxDamage) ? MinDamage : FMath::FRandRange(MinDamage, MaxDamage);
	}

	/**
	 * @brief Gets deterministic radial damage for a distance from the damage origin.
	 *
	 * @param Distance Distance from the damage origin in centimeters.
	 * @return Damage amount after inner/outer radius falloff.
	 */
	UFUNCTION(BlueprintPure, Category = "Damage Effect")
	float GetDamageAmountAtDistance(float Distance) const
	{
		const float MinDamage = FMath::Min(DamageLowerBound, DamageUpperBound);
		const float MaxDamage = FMath::Max(DamageLowerBound, DamageUpperBound);
		if (OuterRadius <= 0.0f)
		{
			return MaxDamage;
		}

		if (Distance <= FMath::Max(0.0f, InnerRadius))
		{
			return MaxDamage;
		}

		if (Distance >= OuterRadius)
		{
			return MinDamage;
		}

		const float FalloffAlpha = FMath::Clamp((Distance - InnerRadius) / FMath::Max(KINDA_SMALL_NUMBER, OuterRadius - InnerRadius), 0.0f, 1.0f);
		float DamageAlpha = 1.0f - FalloffAlpha;
		switch (FalloffType)
		{
		case EDamageFalloffType::None:
			DamageAlpha = 1.0f;
			break;
		case EDamageFalloffType::Binary:
			DamageAlpha = 0.0f;
			break;
		case EDamageFalloffType::Exponential:
			DamageAlpha = FMath::Square(1.0f - FalloffAlpha);
			break;
		case EDamageFalloffType::CustomCurve:
			DamageAlpha = FalloffCurve ? FMath::Clamp(FalloffCurve->GetFloatValue(FalloffAlpha), 0.0f, 1.0f) : 1.0f - FalloffAlpha;
			break;
		case EDamageFalloffType::Linear:
		default:
			DamageAlpha = 1.0f - FalloffAlpha;
			break;
		}

		return FMath::Lerp(MinDamage, MaxDamage, DamageAlpha);
	}
};
