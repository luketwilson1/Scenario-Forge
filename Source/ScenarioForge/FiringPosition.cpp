// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file FiringPosition.cpp
 * @brief Implements authored tactical firing positions.
 */

#include "FiringPosition.h"

#include "Area.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

/**
 * @brief Initializes this data-only tactical point.
 */
AFiringPosition::AFiringPosition()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorHiddenInGame(true);

	FiringPositionMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FiringPositionMeshComponent"));
	SetRootComponent(FiringPositionMeshComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshFinder.Succeeded())
	{
		FiringPositionMeshComponent->SetStaticMesh(PlaneMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AreaMaterialFinder(TEXT("/Game/M_Area.M_Area"));
	if (AreaMaterialFinder.Succeeded())
	{
		FiringPositionMeshComponent->SetMaterial(0, AreaMaterialFinder.Object);
	}

	FiringPositionMeshComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.0f));
	FiringPositionMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FiringPositionMeshComponent->SetHiddenInGame(true);
}

/**
 * @brief Resets saved runtime visibility so firing-position debug visuals start hidden in game.
 */
void AFiringPosition::PostLoad()
{
	Super::PostLoad();

	Opacity = 1.0f;
	SetActorHiddenInGame(true);
	if (FiringPositionMeshComponent)
	{
		FiringPositionMeshComponent->SetHiddenInGame(true);
		FiringPositionMeshComponent->SetVisibility(true, true);
	}
}

/**
 * @brief Reapplies material parameters when editable firing position data changes.
 *
 * @param Transform Construction transform supplied by Unreal.
 */
void AFiringPosition::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Opacity = 1.0f;
	ApplyFiringPositionMaterial();

	SetActorHiddenInGame(true);
	if (FiringPositionMeshComponent)
	{
		FiringPositionMeshComponent->SetHiddenInGame(true);
		FiringPositionMeshComponent->SetVisibility(true, true);
	}
}

/**
 * @brief Applies the configured firing-position material and parameter values.
 */
void AFiringPosition::ApplyFiringPositionMaterial()
{
	if (!FiringPositionMeshComponent)
	{
		return;
	}

	UMaterialInterface* BaseMaterial = FiringPositionMeshComponent->GetMaterial(0);
	if (!BaseMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(BaseMaterial);
	if (!DynamicMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		FiringPositionMeshComponent->SetMaterial(0, DynamicMaterial);
	}

	DynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), FiringPositionBaseColor);
	DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity);
}
