// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Weapon.cpp
 * @brief Implements runtime weapon customization and projectile firing.
 */

#include "Weapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Projectile.h"
#include "ProjectileCustomization.h"
#include "WeaponCustomization.h"

/**
 * @brief Initializes the weapon actor and its skeletal mesh root.
 */
AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SetRootComponent(SkeletalMeshComponent);
}

/**
 * @brief Applies mesh, animation, material, projectile, and VFX configuration to the weapon.
 *
 * @param WeaponCustomization Customization asset to apply.
 */
void AWeapon::ApplyWeaponCustomization(UWeaponCustomization* WeaponCustomization)
{
	if (!WeaponCustomization || !SkeletalMeshComponent)
	{
		return;
	}

	ActiveWeaponCustomization = WeaponCustomization;

	if (USkeletalMesh* SkeletalMesh = WeaponCustomization->Appearance.SkeletalMesh)
	{
		SkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);
	}

	if (UClass* AnimationBlueprint = WeaponCustomization->Appearance.AnimationBlueprint)
	{
		SkeletalMeshComponent->SetAnimInstanceClass(AnimationBlueprint);
	}

	for (const FMaterialOverride& MaterialOverride : WeaponCustomization->Appearance.MaterialOverrides)
	{
		if (MaterialOverride.Material)
		{
			SkeletalMeshComponent->SetMaterial(MaterialOverride.MaterialSlotIndex, MaterialOverride.Material);
		}
	}
}

/**
 * @brief Fires a single projectile toward the supplied target location.
 *
 * @param TargetLocation World location used to derive the shot direction.
 */
void AWeapon::Fire(const FVector& TargetLocation)
{
	if (!ActiveWeaponCustomization || !SkeletalMeshComponent)
	{
		return;
	}

	const FName MuzzleSocketName = ActiveWeaponCustomization->MuzzleSocketName;
	const FTransform MuzzleTransform = SkeletalMeshComponent->DoesSocketExist(MuzzleSocketName)
		? SkeletalMeshComponent->GetSocketTransform(MuzzleSocketName, RTS_World)
		: SkeletalMeshComponent->GetComponentTransform();

	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const FVector ToTarget = TargetLocation - MuzzleLocation;
	const FVector FireDirection = ToTarget.IsNearlyZero()
		? MuzzleTransform.GetRotation().GetForwardVector()
		: ToTarget.GetSafeNormal();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	/** Spawn muzzle flash from the weapon sheet before spawning the projectile. */
	if (ActiveWeaponCustomization->MuzzleFlashVFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			World,
			ActiveWeaponCustomization->MuzzleFlashVFX,
			MuzzleLocation,
			FireDirection.Rotation());
	}

	if (!ActiveWeaponCustomization->ProjectileCustomization)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = GetInstigator();

	/** Spawn and configure the projectile with its own customization asset. */
	const FTransform ProjectileSpawnTransform(FireDirection.Rotation(), MuzzleLocation);
	AProjectile* Projectile = World->SpawnActor<AProjectile>(AProjectile::StaticClass(), ProjectileSpawnTransform, SpawnParameters);
	if (Projectile)
	{
		Projectile->ApplyProjectileCustomization(ActiveWeaponCustomization->ProjectileCustomization);
		if (Projectile->ProjectileMovementComponent)
		{
			Projectile->ProjectileMovementComponent->Velocity = FireDirection * ActiveWeaponCustomization->ProjectileCustomization->InitialSpeed;
		}
	}
}

/**
 * @brief Runtime startup hook reserved for future weapon initialization.
 */
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
}


