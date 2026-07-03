// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Weapon.cpp
 * @brief Implements runtime weapon customization and projectile firing.
 */

#include "Weapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"
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
 * @brief Gets the customization sheet currently applied to this runtime weapon.
 *
 * @return Active weapon customization sheet, or nullptr when none has been applied.
 */
UWeaponCustomization* AWeapon::GetActiveWeaponCustomization() const
{
	return ActiveWeaponCustomization;
}

/**
 * @brief Gets the transform projectiles and muzzle effects should originate from.
 *
 * @return Muzzle socket world transform, or the weapon transform when the socket cannot be resolved.
 */
FTransform AWeapon::GetMuzzleTransform() const
{
	if (!SkeletalMeshComponent)
	{
		return GetActorTransform();
	}

	const FName MuzzleSocketName = ActiveWeaponCustomization
		? ActiveWeaponCustomization->MuzzleSocketName
		: NAME_None;

	return SkeletalMeshComponent->DoesSocketExist(MuzzleSocketName)
		? SkeletalMeshComponent->GetSocketTransform(MuzzleSocketName, RTS_World)
		: SkeletalMeshComponent->GetComponentTransform();
}

/**
 * @brief Fires a single projectile straight forward from the muzzle socket.
 *
 * @param TargetLocation Unused target point kept for temporary call-site compatibility.
 */
void AWeapon::Fire(const FVector& TargetLocation)
{
	(void)TargetLocation;

	if (!ActiveWeaponCustomization || !SkeletalMeshComponent)
	{
		return;
	}

	const FTransform MuzzleTransform = GetMuzzleTransform();
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const FVector FireDirection = MuzzleTransform.GetRotation().GetForwardVector();

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
 * @brief Fires repeatedly from the muzzle for the requested burst duration.
 *
 * @param BurstDuration Seconds the burst should continue firing.
 */
void AWeapon::FireBurst(float BurstDuration)
{
	if (!ActiveWeaponCustomization)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	StopFireBurst();

	HandleBurstShot();

	if (BurstDuration <= 0.0f || ActiveWeaponCustomization->RateOfFire <= 0)
	{
		StopFireBurst();
		return;
	}

	const float FireInterval = 1.0f / static_cast<float>(ActiveWeaponCustomization->RateOfFire);
	World->GetTimerManager().SetTimer(BurstFireTimerHandle, this, &AWeapon::HandleBurstShot, FireInterval, true);
	World->GetTimerManager().SetTimer(BurstStopTimerHandle, this, &AWeapon::StopFireBurst, BurstDuration, false);
}

/**
 * @brief Stops the active burst.
 */
void AWeapon::StopFireBurst()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurstFireTimerHandle);
		World->GetTimerManager().ClearTimer(BurstStopTimerHandle);
	}

}

/**
 * @brief Fires one shot straight forward from the muzzle.
 */
void AWeapon::HandleBurstShot()
{
	Fire(FVector::ZeroVector);
}

/**
 * @brief Runtime startup hook reserved for future weapon initialization.
 */
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
}


