// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file ScenarioEdMode.cpp
 * @brief Implements the Scenario editor mode.
 */

#include "ScenarioEdMode.h"

#include "Agent.h"
#include "../../ScenarioForge/Area.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "EditorModes.h"
#include "EditorViewportClient.h"
#include "Engine/World.h"
#include "../../ScenarioForge/FiringPosition.h"
#include "InputCoreTypes.h"
#include "../ScenarioHierarchy/SScenarioHierarchyView.h"
#include "Math/RotationMatrix.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Squad.h"
#include "../../ScenarioForge/Zone.h"

#define LOCTEXT_NAMESPACE "ScenarioEdMode"

const FEditorModeID FScenarioEdMode::EM_Scenario = TEXT("ScenarioForge.Scenario");

namespace
{
	constexpr float FiringPositionCoverTraceHeight = 100.0f;
	constexpr float FiringPositionCoverTraceLength = 100.0f;
	constexpr float PerchNormalAngleThresholdDegrees = 45.0f;

	int32 GenerateFiringPositionFlags(UWorld* World, const FHitResult& PlacementHit, const AActor* IgnoreActor)
	{
		int32 GeneratedFlags = static_cast<int32>(EFiringPositionFlag::None);

		const FVector SurfaceNormal = PlacementHit.ImpactNormal.IsNearlyZero()
			? FVector::UpVector
			: FVector(PlacementHit.ImpactNormal).GetSafeNormal();
		const float UpDot = FMath::Clamp(FVector::DotProduct(SurfaceNormal, FVector::UpVector), -1.0f, 1.0f);
		const float NormalAngleDegrees = FMath::RadiansToDegrees(FMath::Acos(UpDot));

		if (NormalAngleDegrees > PerchNormalAngleThresholdDegrees)
		{
			return static_cast<int32>(EFiringPositionFlag::Perch);
		}

		GeneratedFlags |= static_cast<int32>(EFiringPositionFlag::GroundPoint);

		if (!World)
		{
			GeneratedFlags |= static_cast<int32>(EFiringPositionFlag::Open);
			return GeneratedFlags;
		}

		const FVector TraceOrigin = FVector(PlacementHit.ImpactPoint)
			+ FVector::UpVector * FiringPositionCoverTraceHeight
			+ SurfaceNormal * 2.0f;
		const FVector Directions[] =
		{
			FVector::ForwardVector,
			(FVector::ForwardVector + FVector::RightVector).GetSafeNormal(),
			FVector::RightVector,
			(-FVector::ForwardVector + FVector::RightVector).GetSafeNormal(),
			-FVector::ForwardVector,
			(-FVector::ForwardVector - FVector::RightVector).GetSafeNormal(),
			-FVector::RightVector,
			(FVector::ForwardVector - FVector::RightVector).GetSafeNormal()
		};

		bool bRayHits[UE_ARRAY_COUNT(Directions)] = {};
		int32 HitCount = 0;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ScenarioGenerateFiringPositionFlags), true);
		if (IgnoreActor)
		{
			QueryParams.AddIgnoredActor(IgnoreActor);
		}

		for (int32 DirectionIndex = 0; DirectionIndex < UE_ARRAY_COUNT(Directions); ++DirectionIndex)
		{
			FHitResult CoverHit;
			bRayHits[DirectionIndex] = World->LineTraceSingleByObjectType(
				CoverHit,
				TraceOrigin,
				TraceOrigin + Directions[DirectionIndex] * FiringPositionCoverTraceLength,
				FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
				QueryParams);

			if (bRayHits[DirectionIndex])
			{
				++HitCount;
			}
		}

		if (HitCount >= 6)
		{
			GeneratedFlags |= static_cast<int32>(EFiringPositionFlag::Closed);
		}
		else if (HitCount >= 2)
		{
			GeneratedFlags |= static_cast<int32>(EFiringPositionFlag::Partial);
		}
		else
		{
			GeneratedFlags |= static_cast<int32>(EFiringPositionFlag::Open);
		}

		const int32 MajorDirectionIndices[] = { 0, 2, 4, 6 };
		for (const int32 MajorDirectionIndex : MajorDirectionIndices)
		{
			const int32 PreviousIndex = (MajorDirectionIndex + UE_ARRAY_COUNT(Directions) - 1) % UE_ARRAY_COUNT(Directions);
			const int32 NextIndex = (MajorDirectionIndex + 1) % UE_ARRAY_COUNT(Directions);
			if (bRayHits[PreviousIndex] && bRayHits[MajorDirectionIndex] && bRayHits[NextIndex])
			{
				GeneratedFlags |= static_cast<int32>(EFiringPositionFlag::WallLean);
				break;
			}
		}

		return GeneratedFlags;
	}
}

/**
 * @brief Handles viewport key input for scenario placement.
 *
 * @param ViewportClient Viewport client receiving input.
 * @param Viewport Viewport receiving input.
 * @param Key Input key.
 * @param Event Input event type.
 * @return True if handled by Scenario mode.
 */
bool FScenarioEdMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (Key == EKeys::RightMouseButton
		&& Event == IE_Pressed
		&& (PlaceAgentAtCursor(ViewportClient) || PlaceAreaVolumeAtCursor(ViewportClient) || PlaceFiringPositionAtCursor(ViewportClient)))
	{
		return true;
	}

	return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
}

/**
 * @brief Scenario placement uses raw click input rather than transform widgets.
 *
 * @return False because this mode does not use the transform widget.
 */
bool FScenarioEdMode::UsesTransformWidget() const
{
	if (GEditor)
	{
		if (USelection* SelectedActors = GEditor->GetSelectedActors())
		{
			for (FSelectionIterator SelectionIt(*SelectedActors); SelectionIt; ++SelectionIt)
			{
				if (Cast<AArea>(*SelectionIt))
				{
					return true;
				}
			}
		}
	}

	return false;
}

/**
 * @brief Scenario mode can coexist only with default-compatible editor state.
 *
 * @param OtherModeID Other active mode identifier.
 * @return True if this mode is compatible with the other mode.
 */
bool FScenarioEdMode::IsCompatibleWith(FEditorModeID OtherModeID) const
{
	return OtherModeID == FBuiltinEditorModes::EM_Default;
}

/**
 * @brief Places an agent at the viewport cursor and assigns it to the selected squad.
 *
 * @param ViewportClient Viewport client used to determine cursor world location.
 * @return True if an agent was placed.
 */
bool FScenarioEdMode::PlaceAgentAtCursor(FEditorViewportClient* ViewportClient)
{
	ASquad* SelectedSquad = SScenarioHierarchyView::GetActivePlacementSquad();
	if (!IsValid(SelectedSquad) || !ViewportClient || !GEditor)
	{
		return false;
	}

	UWorld* EditorWorld = ViewportClient->GetWorld();
	if (!EditorWorld)
	{
		EditorWorld = GEditor->GetEditorWorldContext().World();
	}

	if (!EditorWorld)
	{
		return false;
	}

	const FViewportCursorLocation CursorLocation = ViewportClient->GetCursorWorldLocationFromMousePos();
	const FVector TraceStart = CursorLocation.GetOrigin();
	const FVector TraceEnd = TraceStart + CursorLocation.GetDirection() * HALF_WORLD_MAX;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ScenarioPlaceAgent), true);
	const bool bHit = EditorWorld->LineTraceSingleByObjectType(
		HitResult,
		TraceStart,
		TraceEnd,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		QueryParams);

	const UCapsuleComponent* DefaultCapsule = AAgent::StaticClass()->GetDefaultObject<AAgent>()->GetCapsuleComponent();
	const float AgentHalfHeight = DefaultCapsule ? DefaultCapsule->GetScaledCapsuleHalfHeight() : 0.0f;
	const FVector SurfaceNormal = bHit ? HitResult.ImpactNormal : FVector::UpVector;
	const FVector SpawnLocation = (bHit ? HitResult.ImpactPoint : GEditor->ClickLocation) + SurfaceNormal * AgentHalfHeight;

	const FScopedTransaction Transaction(LOCTEXT("PlaceAgentInSquadTransaction", "Place Agent in Squad"));
	EditorWorld->Modify();
	SelectedSquad->Modify();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transactional;

	AAgent* NewAgent = EditorWorld->SpawnActor<AAgent>(AAgent::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
	if (!NewAgent)
	{
		return false;
	}

	const FString AgentLabel = FString::Printf(TEXT("%s Agent %d"), *SelectedSquad->GetActorLabel(), SelectedSquad->Agents.Num() + 1);
	NewAgent->SetAgentName(AgentLabel);
	NewAgent->Modify();

	SelectedSquad->Agents.Add(NewAgent);

	GEditor->SelectNone(false, true);
	GEditor->SelectActor(NewAgent, true, true, true);

	SScenarioHierarchyView::BroadcastScenarioDataChanged();
	return true;
}

/**
 * @brief Places an area volume at the viewport cursor and assigns it to the selected zone.
 *
 * @param ViewportClient Viewport client used to determine cursor world location.
 * @return True if an area volume was placed.
 */
bool FScenarioEdMode::PlaceAreaVolumeAtCursor(FEditorViewportClient* ViewportClient)
{
	AZone* SelectedZone = SScenarioHierarchyView::GetActivePlacementZone();
	if (!IsValid(SelectedZone) || !ViewportClient || !GEditor)
	{
		return false;
	}

	UWorld* EditorWorld = ViewportClient->GetWorld();
	if (!EditorWorld)
	{
		EditorWorld = GEditor->GetEditorWorldContext().World();
	}

	if (!EditorWorld)
	{
		return false;
	}

	const FViewportCursorLocation CursorLocation = ViewportClient->GetCursorWorldLocationFromMousePos();
	const FVector TraceStart = CursorLocation.GetOrigin();
	const FVector TraceEnd = TraceStart + CursorLocation.GetDirection() * HALF_WORLD_MAX;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ScenarioPlaceAreaVolume), true);
	const bool bHit = EditorWorld->LineTraceSingleByObjectType(
		HitResult,
		TraceStart,
		TraceEnd,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		QueryParams);

	constexpr float AreaHalfHeight = 100.0f;
	const FVector SurfaceNormal = bHit ? HitResult.ImpactNormal : FVector::UpVector;
	const FVector SpawnLocation = (bHit ? HitResult.ImpactPoint : GEditor->ClickLocation) + SurfaceNormal * AreaHalfHeight;

	const FScopedTransaction Transaction(LOCTEXT("PlaceAreaInZoneTransaction", "Place Area Volume in Zone"));
	EditorWorld->Modify();
	SelectedZone->Modify();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transactional;

	AArea* NewArea = EditorWorld->SpawnActor<AArea>(AArea::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
	if (!NewArea)
	{
		return false;
	}

	const FString AreaLabel = FString::Printf(TEXT("%s Area %d"), *SelectedZone->GetActorLabel(), SelectedZone->Areas.Num() + 1);
	NewArea->AreaName = FName(*AreaLabel);
	NewArea->AreaBaseColor = FLinearColor(FMath::FRand(), FMath::FRand(), FMath::FRand(), 1.0f);
	NewArea->SetActorLabel(AreaLabel);
	NewArea->ApplyAreaMaterial();
	NewArea->Modify();

	SelectedZone->Areas.Add(NewArea);

	GEditor->SelectNone(false, true);
	GEditor->SelectActor(NewArea, true, true, true);
	SScenarioHierarchyView::SetActivePlacementArea(NewArea);

	SScenarioHierarchyView::BroadcastScenarioDataChanged();
	return true;
}

/**
 * @brief Places a firing position plane inside the selected area.
 *
 * @param ViewportClient Viewport client used to determine cursor world location.
 * @return True if a firing position was placed.
 */
bool FScenarioEdMode::PlaceFiringPositionAtCursor(FEditorViewportClient* ViewportClient)
{
	AArea* SelectedArea = SScenarioHierarchyView::GetActivePlacementArea();
	if (!IsValid(SelectedArea) || !ViewportClient || !GEditor || !SelectedArea->AreaMeshComponent)
	{
		return false;
	}

	UWorld* EditorWorld = ViewportClient->GetWorld();
	if (!EditorWorld)
	{
		EditorWorld = GEditor->GetEditorWorldContext().World();
	}

	if (!EditorWorld)
	{
		return false;
	}

	const FViewportCursorLocation CursorLocation = ViewportClient->GetCursorWorldLocationFromMousePos();
	const FVector TraceStart = CursorLocation.GetOrigin();
	const FVector TraceEnd = TraceStart + CursorLocation.GetDirection() * HALF_WORLD_MAX;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ScenarioPlaceFiringPosition), true);
	const bool bHit = EditorWorld->LineTraceSingleByObjectType(
		HitResult,
		TraceStart,
		TraceEnd,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		QueryParams);

	if (!bHit || !SelectedArea->AreaMeshComponent->Bounds.GetBox().IsInsideOrOn(HitResult.ImpactPoint))
	{
		return false;
	}

	const FVector SurfaceNormal = HitResult.ImpactNormal.IsNearlyZero() ? FVector::UpVector : FVector(HitResult.ImpactNormal).GetSafeNormal();
	const FVector SpawnLocation = FVector(HitResult.ImpactPoint) + SurfaceNormal;
	const FRotator SpawnRotation = FRotationMatrix::MakeFromZX(SurfaceNormal, SelectedArea->GetActorForwardVector()).Rotator();
	const int32 GeneratedPositionFlags = GenerateFiringPositionFlags(EditorWorld, HitResult, SelectedArea);

	const FScopedTransaction Transaction(LOCTEXT("PlaceFiringPositionInAreaTransaction", "Place Firing Position in Area"));
	EditorWorld->Modify();
	SelectedArea->Modify();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transactional;

	AFiringPosition* NewFiringPosition = EditorWorld->SpawnActor<AFiringPosition>(
		AFiringPosition::StaticClass(),
		SpawnLocation,
		SpawnRotation,
		SpawnParameters);

	if (!NewFiringPosition)
	{
		return false;
	}

	const FString FiringPositionLabel = FString::Printf(TEXT("%s Firing Position %d"), *SelectedArea->GetActorLabel(), SelectedArea->FiringPositions.Num() + 1);
	NewFiringPosition->SetActorLabel(FiringPositionLabel);
	NewFiringPosition->FiringPositionBaseColor = SelectedArea->AreaBaseColor;
	NewFiringPosition->Opacity = FMath::Clamp(SelectedArea->Opacity + 0.05f, 0.0f, 1.0f);
	NewFiringPosition->PositionFlags = GeneratedPositionFlags;
	NewFiringPosition->ApplyFiringPositionMaterial();
	NewFiringPosition->Modify();

	SelectedArea->FiringPositions.Add(NewFiringPosition);

	GEditor->SelectNone(false, true);
	GEditor->SelectActor(NewFiringPosition, true, true, true);

	SScenarioHierarchyView::BroadcastScenarioDataChanged();
	return true;
}

#undef LOCTEXT_NAMESPACE
