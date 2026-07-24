// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Weapon.cpp
 * @brief Implements runtime weapon sheet and projectile firing.
 */

#include "Weapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Projectile.h"
#include "ProjectileSheet.h"
#include "WeaponSheet.h"

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
 * @param WeaponSheet Sheet asset to apply.
 */
void AWeapon::ApplyWeaponSheet(UWeaponSheet* WeaponSheet)
{
	if (!WeaponSheet || !SkeletalMeshComponent)
	{
		return;
	}

	ActiveWeaponSheet = WeaponSheet;

	if (USkeletalMesh* SkeletalMesh = WeaponSheet->Appearance.SkeletalMesh)
	{
		SkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);
	}

	if (UClass* AnimationBlueprint = WeaponSheet->Appearance.AnimationBlueprint)
	{
		SkeletalMeshComponent->SetAnimInstanceClass(AnimationBlueprint);
	}

	for (const FMaterialOverride& MaterialOverride : WeaponSheet->Appearance.MaterialOverrides)
	{
		if (MaterialOverride.Material)
		{
			SkeletalMeshComponent->SetMaterial(MaterialOverride.MaterialSlotIndex, MaterialOverride.Material);
		}
	}
}

/**
 * @brief Gets the sheet sheet currently applied to this runtime weapon.
 *
 * @return Active weapon sheet sheet, or nullptr when none has been applied.
 */
UWeaponSheet* AWeapon::GetActiveWeaponSheet() const
{
	return ActiveWeaponSheet;
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

	const FName MuzzleSocketName = ActiveWeaponSheet
		? ActiveWeaponSheet->MuzzleSocketName
		: NAME_None;

	return SkeletalMeshComponent->DoesSocketExist(MuzzleSocketName)
		? SkeletalMeshComponent->GetSocketTransform(MuzzleSocketName, RTS_World)
		: SkeletalMeshComponent->GetComponentTransform();
}

/**
 * @brief Fires a single projectile from the muzzle toward the target actor.
 *
 * @param TargetActor Actor whose current location supplies the desired trajectory.
 * @param AllowableAimDeviation Maximum permitted muzzle-to-target angle in degrees.
 * @return True when a projectile was fired.
 */
bool AWeapon::FireAtTarget(const AActor* TargetActor, const float AllowableAimDeviation)
{
	if (!ActiveWeaponSheet || !SkeletalMeshComponent || !IsValid(TargetActor))
	{
		return false;
	}

	const FTransform MuzzleTransform = GetMuzzleTransform();
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const FVector FireDirection = (TargetActor->GetActorLocation() - MuzzleLocation).GetSafeNormal();
	if (FireDirection.IsNearlyZero() || !IsMuzzleAlignedWithTarget(TargetActor, AllowableAimDeviation))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	/** Spawn muzzle flash from the weapon sheet before spawning the projectile. */
	if (ActiveWeaponSheet->MuzzleFlashVFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			World,
			ActiveWeaponSheet->MuzzleFlashVFX,
			MuzzleLocation,
			FireDirection.Rotation());
	}

	if (!ActiveWeaponSheet->ProjectileSheet)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = GetInstigator();

	/** Spawn and configure the projectile with its own sheet asset. */
	const FTransform ProjectileSpawnTransform(FireDirection.Rotation(), MuzzleLocation);
	AProjectile* Projectile = World->SpawnActor<AProjectile>(AProjectile::StaticClass(), ProjectileSpawnTransform, SpawnParameters);
	if (Projectile)
	{
		Projectile->ApplyProjectileSheet(ActiveWeaponSheet->ProjectileSheet);
		Projectile->Launch(FireDirection * ActiveWeaponSheet->ProjectileSheet->InitialSpeed);
		return true;
	}

	return false;
}

bool AWeapon::IsMuzzleAlignedWithTarget(const AActor* TargetActor, const float AllowableAimDeviation) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	/** Zero disables the visual firing gate so inherited sheets remain backward compatible. */
	if (AllowableAimDeviation <= 0.0f)
	{
		return true;
	}

	const FTransform MuzzleTransform = GetMuzzleTransform();
	const FVector DesiredDirection = (TargetActor->GetActorLocation() - MuzzleTransform.GetLocation()).GetSafeNormal();
	if (DesiredDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector MuzzleForward = MuzzleTransform.GetRotation().GetForwardVector().GetSafeNormal();
	const float MaximumDeviation = FMath::Clamp(AllowableAimDeviation, 0.0f, 180.0f);
	const float MinimumAlignmentDot = FMath::Cos(FMath::DegreesToRadians(MaximumDeviation));
	return FVector::DotProduct(MuzzleForward, DesiredDirection) >= MinimumAlignmentDot;
}

/**
 * @brief Fires repeatedly from the muzzle for the requested burst duration.
 *
 * @param BurstDuration Seconds the burst should continue firing.
 * @param OnFinished Callback invoked after a timed burst finishes.
 */
void AWeapon::FireBurst(
	AActor* TargetActor,
	const float AllowableAimDeviation,
	const float BurstDuration,
	FOnWeaponBurstFinished OnFinished)
{
	if (!ActiveWeaponSheet || !IsValid(TargetActor))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	StopFireBurst();
	BurstTargetActor = TargetActor;
	BurstAllowableAimDeviation = FMath::Clamp(AllowableAimDeviation, 0.0f, 180.0f);

	HandleBurstShot();

	if (BurstDuration <= 0.0f || ActiveWeaponSheet->RateOfFire <= 0)
	{
		BurstTargetActor.Reset();
		return;
	}

	BurstFinishedDelegate = MoveTemp(OnFinished);
	const float FireInterval = 1.0f / static_cast<float>(ActiveWeaponSheet->RateOfFire);
	World->GetTimerManager().SetTimer(BurstFireTimerHandle, this, &AWeapon::HandleBurstShot, FireInterval, true);
	World->GetTimerManager().SetTimer(BurstStopTimerHandle, this, &AWeapon::FinishFireBurst, BurstDuration, false);
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
	BurstFinishedDelegate.Unbind();
	BurstTargetActor.Reset();

}

/**
 * @brief Fires one shot straight forward from the muzzle.
 */
void AWeapon::HandleBurstShot()
{
	FireAtTarget(BurstTargetActor.Get(), BurstAllowableAimDeviation);
}

/**
 * @brief Ends the active timed burst and invokes its completion callback.
 */
void AWeapon::FinishFireBurst()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurstFireTimerHandle);
		World->GetTimerManager().ClearTimer(BurstStopTimerHandle);
	}

	FOnWeaponBurstFinished CompletionDelegate = MoveTemp(BurstFinishedDelegate);
	BurstTargetActor.Reset();
	CompletionDelegate.ExecuteIfBound();
}

/**
 * @brief Runtime startup hook reserved for future weapon initialization.
 */
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
}


