// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentCustomization.cpp
 * @brief Provides the translation unit for agent customization data assets.
 */

#include "AgentCustomization.h"

#include "Goal.h"
#include "PawnCustomization.h"
#include "QueryCurrentTargetDead.h"
#include "ScenarioForgeGameplayTags.h"

namespace
{
	const FAppearance EmptyAppearance;
	const FAimingProperties DefaultAimingProperties;
	const FDodgeProperties DefaultDodgeProperties;
	const FFiredUponProperties DefaultFiredUponProperties;

	bool CanResolveFromParent(const UAgentCustomization* Customization, TSet<const UAgentCustomization*>& Visited)
	{
		if (!Customization || !Customization->Parent || Visited.Contains(Customization))
		{
			return false;
		}

		Visited.Add(Customization);
		return !Visited.Contains(Customization->Parent);
	}
}

UAgentCustomization::UAgentCustomization()
{
	StateQueries.Add(TAG_State_DestroyTarget.GetTag(), UQueryCurrentTargetDead::StaticClass());
}

const TMap<TSubclassOf<UAction>, float>& UAgentCustomization::GetResolvedActions() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedActions(Visited);
}

TMap<TSubclassOf<UGoal>, float> UAgentCustomization::GetResolvedGoals() const
{
	TSet<const UAgentCustomization*> Visited;
	TMap<TSubclassOf<UGoal>, float> ResolvedGoals;
	AppendResolvedGoals(Visited, ResolvedGoals);
	return ResolvedGoals;
}

bool UAgentCustomization::GetResolvedDrawGoalChangeDebug() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedDrawGoalChangeDebug(Visited);
}

bool UAgentCustomization::GetResolvedDrawStateChangeDebug() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedDrawStateChangeDebug(Visited);
}

const TMap<FGameplayTag, TSubclassOf<UWorldStateQuery>>& UAgentCustomization::GetResolvedStateQueries() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedStateQueries(Visited);
}

const TMap<EGrenadeType, FGrenadeProperties>& UAgentCustomization::GetResolvedGrenadeProperties() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedGrenadeProperties(Visited);
}

const FGrenadeProperties* UAgentCustomization::FindResolvedGrenadeProperties(EGrenadeType GrenadeType) const
{
	return GrenadeType != EGrenadeType::None
		? GetResolvedGrenadeProperties().Find(GrenadeType)
		: nullptr;
}

const TArray<FStartingGrenade>& UAgentCustomization::GetResolvedStartingGrenades() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedStartingGrenades(Visited);
}

UWeaponCustomization* UAgentCustomization::GetResolvedDefaultPrimaryWeapon() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedDefaultPrimaryWeapon(Visited);
}

UWeaponCustomization* UAgentCustomization::GetResolvedDefaultSecondaryWeapon() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedDefaultSecondaryWeapon(Visited);
}

const UPawnCustomization* UAgentCustomization::GetResolvedPawnCustomization() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedPawnCustomization(Visited);
}

const FAppearance& UAgentCustomization::GetResolvedAppearance() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedAppearance(Visited);
}

EFaction UAgentCustomization::GetResolvedFaction() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedFaction(Visited);
}

const FPerception& UAgentCustomization::GetResolvedPerception() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedPerception(Visited);
}

const FAimingProperties& UAgentCustomization::GetResolvedAimingProperties() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedAimingProperties(Visited);
}

const FEngageProperties& UAgentCustomization::GetResolvedEngageProperties() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedEngageProperties(Visited);
}

const FCoverProperties& UAgentCustomization::GetResolvedCoverProperties() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedCoverProperties(Visited);
}

const FDodgeProperties& UAgentCustomization::GetResolvedDodgeProperties() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedDodgeProperties(Visited);
}

const FFiredUponProperties& UAgentCustomization::GetResolvedFiredUponProperties() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedFiredUponProperties(Visited);
}

const FVitalityProperties& UAgentCustomization::GetResolvedVitalityProperties() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedVitalityProperties(Visited);
}

const TMap<UWeaponCustomization*, FWeaponProperties>& UAgentCustomization::GetResolvedWeaponProperties() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedWeaponProperties(Visited);
}

const FWeaponProperties* UAgentCustomization::FindResolvedWeaponProperties(const UWeaponCustomization* WeaponCustomization) const
{
	return WeaponCustomization
		? GetResolvedWeaponProperties().Find(const_cast<UWeaponCustomization*>(WeaponCustomization))
		: nullptr;
}

const TMap<TSubclassOf<UAction>, float>& UAgentCustomization::GetResolvedActions(TSet<const UAgentCustomization*>& Visited) const
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
void UAgentCustomization::AppendResolvedGoals(TSet<const UAgentCustomization*>& Visited, TMap<TSubclassOf<UGoal>, float>& OutGoals) const
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

bool UAgentCustomization::GetResolvedDrawGoalChangeDebug(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideDrawGoalChangeDebug && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDrawGoalChangeDebug(Visited);
	}

	return bDrawGoalChangeDebug;
}

bool UAgentCustomization::GetResolvedDrawStateChangeDebug(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideDrawStateChangeDebug && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDrawStateChangeDebug(Visited);
	}

	return bDrawStateChangeDebug;
}

const TMap<FGameplayTag, TSubclassOf<UWorldStateQuery>>& UAgentCustomization::GetResolvedStateQueries(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideStateQueries && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedStateQueries(Visited);
	}

	return StateQueries;
}

const TMap<EGrenadeType, FGrenadeProperties>& UAgentCustomization::GetResolvedGrenadeProperties(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideGrenadeProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedGrenadeProperties(Visited);
	}

	return GrenadeProperties;
}

const TArray<FStartingGrenade>& UAgentCustomization::GetResolvedStartingGrenades(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideStartingGrenades && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedStartingGrenades(Visited);
	}

	return StartingGrenades;
}

UWeaponCustomization* UAgentCustomization::GetResolvedDefaultPrimaryWeapon(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideDefaultPrimaryWeapon && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDefaultPrimaryWeapon(Visited);
	}

	return DefaultPrimaryWeapon;
}

UWeaponCustomization* UAgentCustomization::GetResolvedDefaultSecondaryWeapon(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideDefaultSecondaryWeapon && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDefaultSecondaryWeapon(Visited);
	}

	return DefaultSecondaryWeapon;
}

const UPawnCustomization* UAgentCustomization::GetResolvedPawnCustomization(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverridePawnCustomization && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedPawnCustomization(Visited);
	}

	return PawnCustomization;
}

const FAppearance& UAgentCustomization::GetResolvedAppearance(TSet<const UAgentCustomization*>& Visited) const
{
	const UPawnCustomization* ResolvedPawnCustomization = GetResolvedPawnCustomization(Visited);
	return ResolvedPawnCustomization ? ResolvedPawnCustomization->Appearance : EmptyAppearance;
}

EFaction UAgentCustomization::GetResolvedFaction(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideFaction && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedFaction(Visited);
	}

	return Faction;
}

const FPerception& UAgentCustomization::GetResolvedPerception(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverridePerception && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedPerception(Visited);
	}

	return Perception;
}

const FAimingProperties& UAgentCustomization::GetResolvedAimingProperties(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideAimingProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedAimingProperties(Visited);
	}

	return bOverrideAimingProperties || !Parent ? AimingProperties : DefaultAimingProperties;
}

const FEngageProperties& UAgentCustomization::GetResolvedEngageProperties(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideEngageProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedEngageProperties(Visited);
	}

	return EngageProperties;
}

const FCoverProperties& UAgentCustomization::GetResolvedCoverProperties(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideCoverProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedCoverProperties(Visited);
	}

	return CoverProperties;
}

const FDodgeProperties& UAgentCustomization::GetResolvedDodgeProperties(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideDodgeProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedDodgeProperties(Visited);
	}

	return bOverrideDodgeProperties || !Parent ? DodgeProperties : DefaultDodgeProperties;
}

const FFiredUponProperties& UAgentCustomization::GetResolvedFiredUponProperties(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideFiredUponProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedFiredUponProperties(Visited);
	}

	return bOverrideFiredUponProperties || !Parent ? FiredUponProperties : DefaultFiredUponProperties;
}

const FVitalityProperties& UAgentCustomization::GetResolvedVitalityProperties(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideVitalityProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedVitalityProperties(Visited);
	}

	return VitalityProperties;
}

const TMap<UWeaponCustomization*, FWeaponProperties>& UAgentCustomization::GetResolvedWeaponProperties(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideWeaponProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedWeaponProperties(Visited);
	}

	return WeaponProperties;
}
