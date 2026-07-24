// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file EquipmentComponent.cpp
 * @brief Implements the component that stores equipment held by an actor.
 */

#include "EquipmentComponent.h"

namespace
{
	bool IsSelectableGrenadeType(const EGrenadeType GrenadeType)
	{
		return GrenadeType != EGrenadeType::None;
	}
}

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UEquipmentComponent::GetGrenadeCount(const EGrenadeType GrenadeType) const
{
	const int32* Count = HeldGrenades.Find(GrenadeType);
	return Count ? FMath::Max(0, *Count) : 0;
}

bool UEquipmentComponent::HasGrenade(const EGrenadeType GrenadeType) const
{
	return IsSelectableGrenadeType(GrenadeType) && GetGrenadeCount(GrenadeType) > 0;
}

bool UEquipmentComponent::HasAnyGrenade() const
{
	for (const TPair<EGrenadeType, int32>& GrenadeEntry : HeldGrenades)
	{
		if (IsSelectableGrenadeType(GrenadeEntry.Key) && GrenadeEntry.Value > 0)
		{
			return true;
		}
	}

	return false;
}

bool UEquipmentComponent::HasCurrentGrenade() const
{
	return HasGrenade(CurrentGrenadeType);
}

void UEquipmentComponent::AddGrenades(const EGrenadeType GrenadeType, const int32 Count)
{
	if (!IsSelectableGrenadeType(GrenadeType) || Count <= 0)
	{
		return;
	}

	const int32 NewCount = GetGrenadeCount(GrenadeType) + Count;
	HeldGrenades.Add(GrenadeType, NewCount);
	RefreshCurrentGrenadeType();
}

bool UEquipmentComponent::ConsumeGrenades(const EGrenadeType GrenadeType, const int32 Count)
{
	if (Count <= 0)
	{
		return true;
	}

	if (!IsSelectableGrenadeType(GrenadeType))
	{
		return false;
	}

	const int32 CurrentCount = GetGrenadeCount(GrenadeType);
	if (CurrentCount < Count)
	{
		return false;
	}

	const int32 NewCount = CurrentCount - Count;
	if (NewCount > 0)
	{
		HeldGrenades.Add(GrenadeType, NewCount);
	}
	else
	{
		HeldGrenades.Remove(GrenadeType);
	}

	RefreshCurrentGrenadeType();
	return true;
}

bool UEquipmentComponent::RefreshCurrentGrenadeType()
{
	if (HasCurrentGrenade())
	{
		return true;
	}

	for (const TPair<EGrenadeType, int32>& GrenadeEntry : HeldGrenades)
	{
		if (IsSelectableGrenadeType(GrenadeEntry.Key) && GrenadeEntry.Value > 0)
		{
			CurrentGrenadeType = GrenadeEntry.Key;
			return true;
		}
	}

	CurrentGrenadeType = EGrenadeType::None;
	return false;
}

void UEquipmentComponent::SetCurrentGrenadeType(const EGrenadeType GrenadeType)
{
	CurrentGrenadeType = GrenadeType;
	RefreshCurrentGrenadeType();
}

UEquipmentSheet* UEquipmentComponent::GetGrenadeEquipment(const EGrenadeType GrenadeType) const
{
	const TObjectPtr<UEquipmentSheet>* Equipment = HeldGrenadeEquipment.Find(GrenadeType);
	return Equipment ? Equipment->Get() : nullptr;
}
