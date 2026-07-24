// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AI/AgentSheet.cpp
 * @brief Provides the translation unit for agent sheet data assets.
 */

#include "AgentSheet.h"

#include "Goal.h"
#include "PawnSheet.h"

namespace
{
	const FAppearance EmptyAppearance;
	const FAimingProperties DefaultAimingProperties;
	const FMeleeProperties DefaultMeleeProperties;
	const FDodgeProperties DefaultDodgeProperties;
	const FFiredUponProperties DefaultFiredUponProperties;

	bool CanResolveFromParent(const UAgentSheet* Sheet, TSet<const UAgentSheet*>& Visited)
	{
		if (!Sheet || !Sheet->Parent || Visited.Contains(Sheet))
		{
			return false;
		}

		Visited.Add(Sheet);
		return !Visited.Contains(Sheet->Parent);
	}
}

const TMap<TSubclassOf<UAction>, float>& UAgentSheet::GetResolvedActions() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedActions(Visited);
}

TMap<TSubclassOf<UGoal>, float> UAgentSheet::GetResolvedGoals() const
{
	TSet<const UAgentSheet*> Visited;
	TMap<TSubclassOf<UGoal>, float> ResolvedGoals;
	AppendResolvedGoals(Visited, ResolvedGoals);
	return ResolvedGoals;
}

bool UAgentSheet::GetResolvedDrawGoalChangeDebug() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedDrawGoalChangeDebug(Visited);
}

bool UAgentSheet::GetResolvedDrawStateChangeDebug() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedDrawStateChangeDebug(Visited);
}

bool UAgentSheet::GetResolvedDrawStationaryTargetDebug() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedDrawStationaryTargetDebug(Visited);
}

const TMap<EGrenadeType, FGrenadeProperties>& UAgentSheet::GetResolvedGrenadeProperties() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedGrenadeProperties(Visited);
}

const FGrenadeProperties* UAgentSheet::FindResolvedGrenadeProperties(EGrenadeType GrenadeType) const
{
	return GrenadeType != EGrenadeType::None
		? GetResolvedGrenadeProperties().Find(GrenadeType)
		: nullptr;
}

const TArray<FStartingGrenade>& UAgentSheet::GetResolvedStartingGrenades() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedStartingGrenades(Visited);
}

UWeaponSheet* UAgentSheet::GetResolvedDefaultPrimaryWeapon() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedDefaultPrimaryWeapon(Visited);
}

UWeaponSheet* UAgentSheet::GetResolvedDefaultSecondaryWeapon() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedDefaultSecondaryWeapon(Visited);
}

const UPawnSheet* UAgentSheet::GetResolvedPawnSheet() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedPawnSheet(Visited);
}

const FAppearance& UAgentSheet::GetResolvedAppearance() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedAppearance(Visited);
}

EFaction UAgentSheet::GetResolvedFaction() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedFaction(Visited);
}

const FPerception& UAgentSheet::GetResolvedPerception() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedPerception(Visited);
}

const FAimingProperties& UAgentSheet::GetResolvedAimingProperties() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedAimingProperties(Visited);
}

const FMeleeProperties& UAgentSheet::GetResolvedMeleeProperties() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedMeleeProperties(Visited);
}

const FEngageProperties& UAgentSheet::GetResolvedEngageProperties() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedEngageProperties(Visited);
}

const FCoverProperties& UAgentSheet::GetResolvedCoverProperties() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedCoverProperties(Visited);
}

const FDodgeProperties& UAgentSheet::GetResolvedDodgeProperties() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedDodgeProperties(Visited);
}

const FFiredUponProperties& UAgentSheet::GetResolvedFiredUponProperties() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedFiredUponProperties(Visited);
}

const FVitalityProperties& UAgentSheet::GetResolvedVitalityProperties() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedVitalityProperties(Visited);
}

const TMap<UWeaponSheet*, FWeaponProperties>& UAgentSheet::GetResolvedWeaponProperties() const
{
	TSet<const UAgentSheet*> Visited;
	return GetResolvedWeaponProperties(Visited);
}

const FWeaponProperties* UAgentSheet::FindResolvedWeaponProperties(const UWeaponSheet* WeaponSheet) const
{
	return WeaponSheet
		? GetResolvedWeaponProperties().Find(const_cast<UWeaponSheet*>(WeaponSheet))
		: nullptr;
}

const TMap<TSubclassOf<UAction>, float>& UAgentSheet::GetResolvedActions(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideActions && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedActions(Visited);
	}

	return Actions;
}

/**
 * @brief Appends inherited and local goals in parent-first order.
 *
 * @param Visited Agent sheets already traversed in the current inheritance chain.
 * @param OutGoals Receives unique, non-null goal subclasses and their scores.
 */
void UAgentSheet::AppendResolvedGoals(TSet<const UAgentSheet*>& Visited, TMap<TSubclassOf<UGoal>, float>& OutGoals) const
{
	if (Visited.Contains(this))
	{
		return;
	}

	Visited.Add(this);
	if (Parent)
	{
		Parent->AppendResolvedGoals(Visited, OutGoals);
	}

	for (const TPair<TSubclassOf<UGoal>, float>& GoalEntry : Goals)
	{
		if (GoalEntry.Key)
		{
			OutGoals.Add(GoalEntry.Key, GoalEntry.Value);
		}
	}
}

bool UAgentSheet::GetResolvedDrawGoalChangeDebug(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideDrawGoalChangeDebug && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDrawGoalChangeDebug(Visited);
	}

	return bDrawGoalChangeDebug;
}

bool UAgentSheet::GetResolvedDrawStateChangeDebug(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideDrawStateChangeDebug && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDrawStateChangeDebug(Visited);
	}

	return bDrawStateChangeDebug;
}

bool UAgentSheet::GetResolvedDrawStationaryTargetDebug(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideDrawStationaryTargetDebug && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDrawStationaryTargetDebug(Visited);
	}

	return bDrawStationaryTargetDebug;
}

const TMap<EGrenadeType, FGrenadeProperties>& UAgentSheet::GetResolvedGrenadeProperties(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideGrenadeProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedGrenadeProperties(Visited);
	}

	return GrenadeProperties;
}

const TArray<FStartingGrenade>& UAgentSheet::GetResolvedStartingGrenades(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideStartingGrenades && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedStartingGrenades(Visited);
	}

	return StartingGrenades;
}

UWeaponSheet* UAgentSheet::GetResolvedDefaultPrimaryWeapon(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideDefaultPrimaryWeapon && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDefaultPrimaryWeapon(Visited);
	}

	return DefaultPrimaryWeapon;
}

UWeaponSheet* UAgentSheet::GetResolvedDefaultSecondaryWeapon(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideDefaultSecondaryWeapon && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDefaultSecondaryWeapon(Visited);
	}

	return DefaultSecondaryWeapon;
}

const UPawnSheet* UAgentSheet::GetResolvedPawnSheet(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverridePawnSheet && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedPawnSheet(Visited);
	}

	return PawnSheet;
}

const FAppearance& UAgentSheet::GetResolvedAppearance(TSet<const UAgentSheet*>& Visited) const
{
	const UPawnSheet* ResolvedPawnSheet = GetResolvedPawnSheet(Visited);
	return ResolvedPawnSheet ? ResolvedPawnSheet->Appearance : EmptyAppearance;
}

EFaction UAgentSheet::GetResolvedFaction(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideFaction && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedFaction(Visited);
	}

	return Faction;
}

const FPerception& UAgentSheet::GetResolvedPerception(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverridePerception && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedPerception(Visited);
	}

	return Perception;
}

const FAimingProperties& UAgentSheet::GetResolvedAimingProperties(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideAimingProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedAimingProperties(Visited);
	}

	return bOverrideAimingProperties || !Parent ? AimingProperties : DefaultAimingProperties;
}

const FMeleeProperties& UAgentSheet::GetResolvedMeleeProperties(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideMeleeProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedMeleeProperties(Visited);
	}

	return bOverrideMeleeProperties || !Parent ? MeleeProperties : DefaultMeleeProperties;
}

const FEngageProperties& UAgentSheet::GetResolvedEngageProperties(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideEngageProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedEngageProperties(Visited);
	}

	return EngageProperties;
}

const FCoverProperties& UAgentSheet::GetResolvedCoverProperties(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideCoverProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedCoverProperties(Visited);
	}

	return CoverProperties;
}

const FDodgeProperties& UAgentSheet::GetResolvedDodgeProperties(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideDodgeProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDodgeProperties(Visited);
	}

	return bOverrideDodgeProperties || !Parent ? DodgeProperties : DefaultDodgeProperties;
}

const FFiredUponProperties& UAgentSheet::GetResolvedFiredUponProperties(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideFiredUponProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedFiredUponProperties(Visited);
	}

	return bOverrideFiredUponProperties || !Parent ? FiredUponProperties : DefaultFiredUponProperties;
}

const FVitalityProperties& UAgentSheet::GetResolvedVitalityProperties(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideVitalityProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedVitalityProperties(Visited);
	}

	return VitalityProperties;
}

const TMap<UWeaponSheet*, FWeaponProperties>& UAgentSheet::GetResolvedWeaponProperties(TSet<const UAgentSheet*>& Visited) const
{
	if (!bOverrideWeaponProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedWeaponProperties(Visited);
	}

	return WeaponProperties;
}
