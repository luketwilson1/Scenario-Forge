// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file EquipmentComponent.h
 * @brief Declares the component that stores equipment held by an actor.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrenadeTypes.h"
#include "EquipmentComponent.generated.h"

class UEquipmentCustomization;

/**
 * @brief Stores equipment counts and current equipment selections for an actor.
 */
UCLASS(ClassGroup = (Inventory), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SCENARIOFORGE_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Initializes default component settings. */
	UEquipmentComponent();

	/**
	 * @brief Gets the number of grenades held for a grenade type.
	 *
	 * @param GrenadeType Grenade type to inspect.
	 * @return Count held for the grenade type.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment|Grenades")
	int32 GetGrenadeCount(EGrenadeType GrenadeType) const;

	/**
	 * @brief Checks whether this inventory has at least one grenade of a type.
	 *
	 * @param GrenadeType Grenade type to inspect.
	 * @return True when at least one grenade of that type is available.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment|Grenades")
	bool HasGrenade(EGrenadeType GrenadeType) const;

	/**
	 * @brief Checks whether this inventory has any usable grenade type with positive quantity.
	 *
	 * @return True when at least one grenade type has a positive count.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment|Grenades")
	bool HasAnyGrenade() const;

	/**
	 * @brief Checks whether this inventory has at least one currently held grenade.
	 *
	 * @return True when CurrentGrenadeType has a positive count.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment|Grenades")
	bool HasCurrentGrenade() const;

	/**
	 * @brief Adds grenades to the held count for a type.
	 *
	 * @param GrenadeType Grenade type to add.
	 * @param Count Number of grenades to add.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Grenades")
	void AddGrenades(EGrenadeType GrenadeType, int32 Count);

	/**
	 * @brief Consumes grenades from the held count for a type.
	 *
	 * @param GrenadeType Grenade type to consume.
	 * @param Count Number of grenades to consume.
	 * @return True when enough grenades were available and consumed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Grenades")
	bool ConsumeGrenades(EGrenadeType GrenadeType, int32 Count = 1);

	/**
	 * @brief Updates CurrentGrenadeType to a usable grenade, or None when no grenades remain.
	 *
	 * @return True when a usable grenade type is selected after refresh.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Grenades")
	bool RefreshCurrentGrenadeType();

	/**
	 * @brief Sets the grenade type this actor is currently holding.
	 *
	 * @param GrenadeType New current grenade type.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Grenades")
	void SetCurrentGrenadeType(EGrenadeType GrenadeType);

	/**
	 * @brief Gets the grenade type this actor is currently holding.
	 *
	 * @return Current grenade type.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment|Grenades")
	EGrenadeType GetCurrentGrenadeType() const { return CurrentGrenadeType; }

	/** Gets the equipment sheet associated with a grenade type. */
	UFUNCTION(BlueprintPure, Category = "Equipment|Grenades")
	UEquipmentCustomization* GetGrenadeEquipment(EGrenadeType GrenadeType) const;

	/** Grenade counts held by this actor, keyed by grenade type. Empty or zero counts mean no usable grenade. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Grenades", meta = (DisplayName = "Held Grenades"))
	TMap<EGrenadeType, int32> HeldGrenades;

	/** Equipment sheet associated with each held grenade type. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Grenades", meta = (DisplayName = "Held Grenade Equipment"))
	TMap<EGrenadeType, TObjectPtr<UEquipmentCustomization>> HeldGrenadeEquipment;

	/** Grenade type this actor is currently holding or will try to throw. None means no grenade is selected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Grenades", meta = (DisplayName = "Current Grenade Type"))
	EGrenadeType CurrentGrenadeType = EGrenadeType::FragGrenade;
};
