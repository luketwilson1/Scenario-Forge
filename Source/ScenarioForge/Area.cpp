// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Area.cpp
 * @brief Implements authored tactical firing position areas.
 */

#include "Area.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Math/RandomStream.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"
#include "FiringPosition.h"

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
			return static_cast<int32>(EFiringPositionFlag::Perch)
				| static_cast<int32>(EFiringPositionFlag::AutomaticallyGenerated);
		}

		GeneratedFlags |= static_cast<int32>(EFiringPositionFlag::GroundPoint);
		GeneratedFlags |= static_cast<int32>(EFiringPositionFlag::AutomaticallyGenerated);

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
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AreaGenerateFiringPositionFlags), true);
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
 * @brief Initializes this data-only tactical area.
 */
AArea::AArea()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorHiddenInGame(true);

	AreaMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AreaMeshComponent"));
	SetRootComponent(AreaMeshComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		AreaMeshComponent->SetStaticMesh(CubeMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AreaMaterialFinder(TEXT("/Game/M_Area.M_Area"));
	if (AreaMaterialFinder.Succeeded())
	{
		AreaMeshComponent->SetMaterial(0, AreaMaterialFinder.Object);
	}

	AreaMeshComponent->SetRelativeScale3D(FVector(5.0f, 5.0f, 2.0f));
	AreaMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AreaMeshComponent->SetHiddenInGame(true);
}

/**
 * @brief Resets saved runtime visibility so area debug visuals start hidden in game.
 */
void AArea::PostLoad()
{
	Super::PostLoad();

	SetActorHiddenInGame(true);
	if (AreaMeshComponent)
	{
		AreaMeshComponent->SetHiddenInGame(true);
		AreaMeshComponent->SetVisibility(true, true);
	}
}

/**
 * @brief Reapplies material parameters when editable area data changes.
 *
 * @param Transform Construction transform supplied by Unreal.
 */
void AArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyAreaMaterial();

	SetActorHiddenInGame(true);
	if (AreaMeshComponent)
	{
		AreaMeshComponent->SetHiddenInGame(true);
		AreaMeshComponent->SetVisibility(true, true);
	}
}

/**
 * @brief Applies the configured area material and parameter values to the cube mesh.
 */
void AArea::ApplyAreaMaterial()
{
	if (!AreaMeshComponent)
	{
		return;
	}

	UMaterialInterface* BaseMaterial = AreaMeshComponent->GetMaterial(0);
	if (!BaseMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(BaseMaterial);
	if (!DynamicMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		AreaMeshComponent->SetMaterial(0, DynamicMaterial);
	}

	DynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), AreaBaseColor);
	DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity);
}

/**
 * @brief Generates firing positions inside this area using the area generation settings.
 */
void AArea::GenerateFiringPositions()
{
	UWorld* World = GetWorld();
	if (!World || !AreaMeshComponent || GeneratedFiringPositionCount <= 0)
	{
		return;
	}

	Modify();
	World->Modify();

	const FBox AreaBounds = AreaMeshComponent->Bounds.GetBox();
	if (!AreaBounds.IsValid)
	{
		return;
	}

	const FVector BoundsCenter = AreaBounds.GetCenter();
	const FVector BoundsExtent = AreaBounds.GetExtent();
	const float TracePadding = FMath::Max(GeneratedFiringPositionClearanceHalfExtent.Z + 50.0f, 100.0f);
	const float TopZ = AreaBounds.Max.Z + TracePadding;
	const float BottomZ = AreaBounds.Min.Z - TracePadding;
	const float MinSpacingSquared = FMath::Square(FMath::Max(0.0f, GeneratedFiringPositionMinSpacing));

	FRandomStream RandomStream(FMath::Rand());
	const int32 ClusterCount = FMath::Clamp(FMath::CeilToInt(static_cast<float>(GeneratedFiringPositionCount) / 8.0f), 1, 6);
	TArray<FVector2D> ClusterCenters;
	ClusterCenters.Reserve(ClusterCount);
	for (int32 ClusterIndex = 0; ClusterIndex < ClusterCount; ++ClusterIndex)
	{
		ClusterCenters.Add(FVector2D(
			RandomStream.FRandRange(AreaBounds.Min.X, AreaBounds.Max.X),
			RandomStream.FRandRange(AreaBounds.Min.Y, AreaBounds.Max.Y)));
	}

	TArray<FVector> AcceptedLocations;
	AcceptedLocations.Reserve(GeneratedFiringPositionCount);
	for (const AFiringPosition* ExistingPosition : FiringPositions)
	{
		if (IsValid(ExistingPosition))
		{
			AcceptedLocations.Add(ExistingPosition->GetActorLocation());
		}
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AreaGenerateFiringPositions), true);
	QueryParams.AddIgnoredActor(this);
	for (AFiringPosition* ExistingPosition : FiringPositions)
	{
		if (IsValid(ExistingPosition))
		{
			QueryParams.AddIgnoredActor(ExistingPosition);
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transactional;
	SpawnParameters.Owner = this;

	const int32 TargetNewPositions = FMath::Max(0, GeneratedFiringPositionCount - FiringPositions.Num());
	int32 AddedPositions = 0;
	for (int32 AttemptIndex = 0; AttemptIndex < FiringPositionGenerationMaxAttempts && AddedPositions < TargetNewPositions; ++AttemptIndex)
	{
		FVector2D CandidateXY;
		if (ClusterCenters.Num() > 0 && RandomStream.FRand() < 0.7f)
		{
			const FVector2D ClusterCenter = ClusterCenters[RandomStream.RandRange(0, ClusterCenters.Num() - 1)];
			CandidateXY = ClusterCenter + FVector2D(
				RandomStream.GetFraction() * 2.0f - 1.0f,
				RandomStream.GetFraction() * 2.0f - 1.0f) * FVector2D(BoundsExtent.X * 0.35f, BoundsExtent.Y * 0.35f);
		}
		else
		{
			CandidateXY = FVector2D(
				RandomStream.FRandRange(AreaBounds.Min.X, AreaBounds.Max.X),
				RandomStream.FRandRange(AreaBounds.Min.Y, AreaBounds.Max.Y));
		}

		CandidateXY.X = FMath::Clamp(CandidateXY.X, AreaBounds.Min.X, AreaBounds.Max.X);
		CandidateXY.Y = FMath::Clamp(CandidateXY.Y, AreaBounds.Min.Y, AreaBounds.Max.Y);

		FHitResult GroundHit;
		const FVector TraceStart(CandidateXY.X, CandidateXY.Y, TopZ);
		const FVector TraceEnd(CandidateXY.X, CandidateXY.Y, BottomZ);
		if (!World->LineTraceSingleByObjectType(
			GroundHit,
			TraceStart,
			TraceEnd,
			FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
			QueryParams))
		{
			continue;
		}

		if (!AreaBounds.IsInsideOrOn(GroundHit.ImpactPoint))
		{
			continue;
		}

		const FVector SurfaceNormal = GroundHit.ImpactNormal.IsNearlyZero()
			? FVector::UpVector
			: FVector(GroundHit.ImpactNormal).GetSafeNormal();
		const FVector SpawnLocation = FVector(GroundHit.ImpactPoint) + SurfaceNormal * GeneratedFiringPositionSurfaceOffset;

		bool bTooClose = false;
		for (const FVector& AcceptedLocation : AcceptedLocations)
		{
			if (FVector::DistSquared2D(AcceptedLocation, SpawnLocation) < MinSpacingSquared)
			{
				bTooClose = true;
				break;
			}
		}

		if (bTooClose)
		{
			continue;
		}

		const FVector ClearanceCenter = SpawnLocation + SurfaceNormal * GeneratedFiringPositionClearanceHalfExtent.Z;
		TArray<FOverlapResult> Overlaps;
		if (World->OverlapMultiByObjectType(
			Overlaps,
			ClearanceCenter,
			FQuat::Identity,
			FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
			FCollisionShape::MakeBox(GeneratedFiringPositionClearanceHalfExtent),
			QueryParams))
		{
			bool bHasBlockingOverlap = false;
			for (const FOverlapResult& Overlap : Overlaps)
			{
				if (Overlap.bBlockingHit)
				{
					bHasBlockingOverlap = true;
					break;
				}
			}

			if (bHasBlockingOverlap)
			{
				continue;
			}
		}

		const FRotator SpawnRotation = FRotationMatrix::MakeFromZX(SurfaceNormal, GetActorForwardVector()).Rotator();
		AFiringPosition* NewFiringPosition = World->SpawnActor<AFiringPosition>(
			AFiringPosition::StaticClass(),
			SpawnLocation,
			SpawnRotation,
			SpawnParameters);
		if (!NewFiringPosition)
		{
			continue;
		}

		const FString FiringPositionLabel = FString::Printf(TEXT("%s Firing Position %d"), *GetActorLabel(), FiringPositions.Num() + 1);
		NewFiringPosition->SetActorLabel(FiringPositionLabel);
		NewFiringPosition->FiringPositionBaseColor = AreaBaseColor;
		NewFiringPosition->Opacity = 1.0f;
		NewFiringPosition->PositionFlags = GenerateFiringPositionFlags(World, GroundHit, NewFiringPosition);
		NewFiringPosition->ApplyFiringPositionMaterial();
		NewFiringPosition->Modify();

		FiringPositions.Add(NewFiringPosition);
		AcceptedLocations.Add(SpawnLocation);
		QueryParams.AddIgnoredActor(NewFiringPosition);
		++AddedPositions;
	}
}
