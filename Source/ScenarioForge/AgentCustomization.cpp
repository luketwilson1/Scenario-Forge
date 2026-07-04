// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file AgentCustomization.cpp
 * @brief Provides the translation unit for agent customization data assets.
 */

#include "AgentCustomization.h"

#include "PawnCustomization.h"

namespace
{
	const FAppearance EmptyAppearance;

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

const TArray<TObjectPtr<UActionDefinition>>& UAgentCustomization::GetResolvedActions() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedActions(Visited);
}

const FGameplayTagContainer& UAgentCustomization::GetResolvedStartingGoalTags() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedStartingGoalTags(Visited);
}

const FGeneralProperties& UAgentCustomization::GetResolvedGeneralProperties() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedGeneralProperties(Visited);
}

const FGrenadeProperties& UAgentCustomization::GetResolvedGrenadeProperties() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedGrenadeProperties(Visited);
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

const TMap<ETacticalMovementMode, FTacticalMovementModeEvaluatorSet>& UAgentCustomization::GetResolvedTacticalPositionEvaluators() const
{
	TSet<const UAgentCustomization*> Visited;
	return GetResolvedTacticalPositionEvaluators(Visited);
}

const TArray<TObjectPtr<UActionDefinition>>& UAgentCustomization::GetResolvedActions(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideActions && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedActions(Visited);
	}

	return Actions;
}

const FGameplayTagContainer& UAgentCustomization::GetResolvedStartingGoalTags(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideStartingGoalTags && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedStartingGoalTags(Visited);
	}

	return StartingGoalTags;
}

const FGeneralProperties& UAgentCustomization::GetResolvedGeneralProperties(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideGeneralProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedGeneralProperties(Visited);
	}

	return GeneralProperties;
}

const FGrenadeProperties& UAgentCustomization::GetResolvedGrenadeProperties(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideGrenadeProperties && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedGrenadeProperties(Visited);
	}

	return GrenadeProperties;
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

const TMap<ETacticalMovementMode, FTacticalMovementModeEvaluatorSet>& UAgentCustomization::GetResolvedTacticalPositionEvaluators(TSet<const UAgentCustomization*>& Visited) const
{
	if (!bOverrideTacticalPositionEvaluators && CanResolveFromParent(this, Visited))
	{
		return Parent->GetResolvedTacticalPositionEvaluators(Visited);
	}

	return TacticalPositionEvaluators;
}

