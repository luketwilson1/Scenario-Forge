// Copyright (c) 2026 apebyte. All rights reserved. Part of Scenario Forge. Unauthorized use, copying, modification, or distribution is prohibited.

/**
 * @file Projectile.cpp
 * @brief Implements projectile sheet, damage application, impact VFX, and destruction.
 */

#include "Projectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Agent.h"
#include "AgentAIController.h"
#include "DamageEffectSheet.h"
#include "Planner.h"
#include "ProjectileSheet.h"
#include "ScenarioForgeGameplayTags.h"
#include "TimerManager.h"

/**
 * @brief Initializes projectile collision, visual mesh, movement, and lifetime defaults.
 */
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(2.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->SetSimulatePhysics(false);
	CollisionComponent->OnComponentHit.AddDynamic(this, &AProjectile::HandleHit);
	SetRootComponent(CollisionComponent);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(CollisionComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComponent->SetGenerateOverlapEvents(true);
	StaticMeshComponent->SetSimulatePhysics(false);
	StaticMeshComponent->OnComponentHit.AddDynamic(this, &AProjectile::HandleHit);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->InitialSpeed = 10000.0f;
	ProjectileMovementComponent->MaxSpeed = 10000.0f;
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bSweepCollision = true;

	GrenadeDangerComponent = CreateDefaultSubobject<USphereComponent>(TEXT("GrenadeDangerComponent"));
	GrenadeDangerComponent->SetupAttachment(RootComponent);
	GrenadeDangerComponent->InitSphereRadius(1.0f);
	GrenadeDangerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrenadeDangerComponent->SetCollisionObjectType(ECC_WorldDynamic);
	GrenadeDangerComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	GrenadeDangerComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GrenadeDangerComponent->SetGenerateOverlapEvents(false);
	GrenadeDangerComponent->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::HandleGrenadeDangerBeginOverlap);
	GrenadeDangerComponent->OnComponentEndOverlap.AddDynamic(this, &AProjectile::HandleGrenadeDangerEndOverlap);

	InitialLifeSpan = 3.0f;
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDrawGrenadeDangerDebug && GrenadeDangerComponent && GrenadeDangerComponent->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
	{
		DrawDebugSphere(
			GetWorld(),
			GrenadeDangerComponent->GetComponentLocation(),
			GrenadeDangerComponent->GetScaledSphereRadius(),
			32,
			FColor::Orange,
			false,
			0.0f,
			0,
			2.0f);
	}
}

void AProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TWeakObjectPtr<AAgent>& WeakAgent : AgentsInGrenadeDanger)
	{
		if (AAgent* Agent = WeakAgent.Get())
		{
			SetGrenadeDangerState(Agent, false);
		}
	}
	AgentsInGrenadeDanger.Reset();

	Super::EndPlay(EndPlayReason);
}

/**
 * @brief Applies projectile data from a sheet asset.
 *
 * @param ProjectileSheet Data asset containing projectile visuals, movement, damage, and VFX.
 */
void AProjectile::ApplyProjectileSheet(const UProjectileSheet* ProjectileSheet)
{
	if (!ProjectileSheet)
	{
		return;
	}

	if (CollisionComponent)
	{
		CollisionComponent->SetSphereRadius(ProjectileSheet->CollisionRadius, true);
	}

	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetStaticMesh(ProjectileSheet->StaticMesh);
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->InitialSpeed = ProjectileSheet->InitialSpeed;
		ProjectileMovementComponent->MaxSpeed = ProjectileSheet->MaxSpeed;
		ProjectileMovementComponent->ProjectileGravityScale = ProjectileSheet->GravityScale;
	}

	CollisionSource = ProjectileSheet->CollisionSource;
	MovementMode = ProjectileSheet->MovementMode;
	PhysicsAngularVelocity = ProjectileSheet->PhysicsAngularVelocity;
	ConfigureCollisionSource();

	UPrimitiveComponent* ActiveCollisionPrimitive = GetActiveCollisionPrimitive();
	if (ActiveCollisionPrimitive)
	{
		ActiveCollisionPrimitive->SetLinearDamping(ProjectileSheet->PhysicsLinearDamping);
		ActiveCollisionPrimitive->SetAngularDamping(ProjectileSheet->PhysicsAngularDamping);
	}

	DamageEffect = ProjectileSheet->DamageEffect;
	bCreateGrenadeDangerVolume = ProjectileSheet->bCreateGrenadeDangerVolume;
	bDrawGrenadeDangerDebug = false;
	ConfigureGrenadeDangerVolume();
	ImpactVFX = ProjectileSheet->ImpactVFX;
	DetonationVFX = ProjectileSheet->DetonationVFX;
	ImpactBehavior = ProjectileSheet->ImpactBehavior;
	DetonationTrigger = ProjectileSheet->DetonationTrigger;
	DetonationTimer = ProjectileSheet->DetonationTimer;
	bDrawDetonationRadiusDebug = ProjectileSheet->bDrawDetonationRadiusDebug;

	const float ConfiguredLifeSpan = DetonationTrigger == EProjectileDetonationTrigger::Timed && DetonationTimer > 0.0f
		? 0.0f
		: ProjectileSheet->MaxLifetime;
	InitialLifeSpan = ConfiguredLifeSpan;
	SetLifeSpan(ConfiguredLifeSpan);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DetonationTimerHandle);
		if (DetonationTrigger == EProjectileDetonationTrigger::Timed && DetonationTimer > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				DetonationTimerHandle,
				this,
				&AProjectile::Detonate,
				DetonationTimer,
				false);
		}
	}
}

void AProjectile::ConfigureGrenadeDangerVolume()
{
	if (!GrenadeDangerComponent)
	{
		return;
	}

	const float DangerRadius = DamageEffect
		? FMath::Max(DamageEffect->OuterRadius, DamageEffect->InnerRadius)
		: 0.0f;
	const bool bDangerEnabled = bCreateGrenadeDangerVolume && DangerRadius > 0.0f;
	if (RootComponent && GrenadeDangerComponent->GetAttachParent() != RootComponent)
	{
		GrenadeDangerComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	GrenadeDangerComponent->SetRelativeLocation(FVector::ZeroVector);
	GrenadeDangerComponent->SetSphereRadius(FMath::Max(1.0f, DangerRadius), true);
	GrenadeDangerComponent->SetCollisionEnabled(bDangerEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	GrenadeDangerComponent->SetGenerateOverlapEvents(bDangerEnabled);
	SetActorTickEnabled(bDangerEnabled && bDrawGrenadeDangerDebug);
}

void AProjectile::ConfigureCollisionSource()
{
	if (!CollisionComponent || !StaticMeshComponent)
	{
		return;
	}

	CollisionComponent->SetSimulatePhysics(false);
	CollisionComponent->SetUseCCD(false);
	StaticMeshComponent->SetSimulatePhysics(false);
	StaticMeshComponent->SetUseCCD(false);

	if (CollisionSource == EProjectileCollisionSource::StaticMeshCollision && StaticMeshComponent->GetStaticMesh())
	{
		StaticMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		SetRootComponent(StaticMeshComponent);
		CollisionComponent->AttachToComponent(StaticMeshComponent, FAttachmentTransformRules::KeepWorldTransform);

		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComponent->SetNotifyRigidBodyCollision(false);

		StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		StaticMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
		StaticMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
		StaticMeshComponent->SetNotifyRigidBodyCollision(true);

		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->SetUpdatedComponent(StaticMeshComponent);
		}
		return;
	}

	CollisionComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	SetRootComponent(CollisionComponent);
	StaticMeshComponent->AttachToComponent(CollisionComponent, FAttachmentTransformRules::KeepWorldTransform);

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);

	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComponent->SetNotifyRigidBodyCollision(false);

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->SetUpdatedComponent(CollisionComponent);
	}
}

UPrimitiveComponent* AProjectile::GetActiveCollisionPrimitive() const
{
	if (CollisionSource == EProjectileCollisionSource::StaticMeshCollision && StaticMeshComponent && StaticMeshComponent->GetStaticMesh())
	{
		return StaticMeshComponent;
	}

	return CollisionComponent;
}

void AProjectile::Launch(const FVector& InitialVelocity)
{
	UPrimitiveComponent* ActiveCollisionPrimitive = GetActiveCollisionPrimitive();
	if (!ActiveCollisionPrimitive)
	{
		return;
	}

	if (MovementMode == EProjectileMovementMode::SimulatedPhysics)
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->StopMovementImmediately();
			ProjectileMovementComponent->Deactivate();
		}

		if (CollisionSource == EProjectileCollisionSource::SimpleSphere)
		{
			constexpr float MinimumPhysicsCollisionRadius = 5.0f;
			if (CollisionComponent->GetScaledSphereRadius() < MinimumPhysicsCollisionRadius)
			{
				CollisionComponent->SetSphereRadius(MinimumPhysicsCollisionRadius, true);
			}
		}

		ActiveCollisionPrimitive->SetCollisionObjectType(ECC_PhysicsBody);
		ActiveCollisionPrimitive->SetCollisionResponseToAllChannels(ECR_Block);
		ActiveCollisionPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ActiveCollisionPrimitive->SetNotifyRigidBodyCollision(true);
		ActiveCollisionPrimitive->SetUseCCD(true);
		ActiveCollisionPrimitive->SetSimulatePhysics(true);
		ActiveCollisionPrimitive->SetPhysicsLinearVelocity(InitialVelocity);
		ActiveCollisionPrimitive->SetPhysicsAngularVelocityInDegrees(PhysicsAngularVelocity);
		return;
	}

	ActiveCollisionPrimitive->SetCollisionObjectType(ECC_WorldDynamic);
	ActiveCollisionPrimitive->SetCollisionResponseToAllChannels(ECR_Block);
	ActiveCollisionPrimitive->SetUseCCD(false);
	ActiveCollisionPrimitive->SetSimulatePhysics(false);

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Velocity = InitialVelocity;
		ProjectileMovementComponent->InitialSpeed = InitialVelocity.Size();
		ProjectileMovementComponent->MaxSpeed = FMath::Max(ProjectileMovementComponent->MaxSpeed, InitialVelocity.Size());
		ProjectileMovementComponent->Activate(true);
	}
}

float AProjectile::GetDetonationOuterRadius() const
{
	return DamageEffect ? FMath::Max(0.0f, DamageEffect->OuterRadius) : 0.0f;
}

bool AProjectile::IsGrenadeDangerProjectile() const
{
	return bCreateGrenadeDangerVolume;
}

void AProjectile::Detonate()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DetonationTimerHandle);

		/** Keep the radius visible after this projectile actor is destroyed. */
		if (bDrawDetonationRadiusDebug && DamageEffect && DamageEffect->OuterRadius > 0.0f)
		{
			constexpr float DetonationDebugDuration = 5.0f;
			DrawDebugSphere(
				World,
				GetActorLocation(),
				DamageEffect->OuterRadius,
				32,
				FColor::Red,
				false,
				DetonationDebugDuration,
				0,
				3.0f);
		}

		if (DetonationVFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				DetonationVFX,
				GetActorLocation(),
				GetActorRotation());
		}
	}

	ApplyDetonationDamage();
	Destroy();
}

void AProjectile::HandleGrenadeDangerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)OverlappedComponent;
	(void)OtherComponent;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;

	AAgent* Agent = Cast<AAgent>(OtherActor);
	if (!Agent || Agent == GetOwner() || Agent == GetInstigator())
	{
		return;
	}

	AgentsInGrenadeDanger.Add(Agent);
	SetGrenadeDangerState(Agent, true);

}

void AProjectile::SetGrenadeDangerState(AAgent* Agent, bool bInDanger)
{
	if (!Agent)
	{
		return;
	}

	if (bInDanger)
	{
		Agent->AddGrenadeDangerSource(this);
	}
	else
	{
		Agent->RemoveGrenadeDangerSource(this);
	}
	const bool bStillInGrenadeDanger = Agent->HasGrenadeDangerSources();

	if (AAgentAIController* AgentAIController = Cast<AAgentAIController>(Agent->GetController()))
	{
		if (UPlanner* Planner = AgentAIController->GetPlanner())
		{
			if (bStillInGrenadeDanger)
			{
				Planner->AddCurrentState(TAG_State_Danger.GetTag());
				Planner->AddCurrentState(TAG_State_GrenadeNear.GetTag());
			}
			else
			{
				Planner->RemoveCurrentState(TAG_State_GrenadeNear.GetTag());
				Planner->RemoveCurrentState(TAG_State_Danger.GetTag());
			}
		}
	}
}

void AProjectile::HandleGrenadeDangerEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	(void)OverlappedComponent;
	(void)OtherComponent;
	(void)OtherBodyIndex;

	AAgent* Agent = Cast<AAgent>(OtherActor);
	if (!Agent || Agent == GetOwner() || Agent == GetInstigator())
	{
		return;
	}

	AgentsInGrenadeDanger.Remove(Agent);
	SetGrenadeDangerState(Agent, false);
}

void AProjectile::ApplyDetonationDamage()
{
	if (!DamageEffect)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float DamageRadius = FMath::Max(DamageEffect->OuterRadius, DamageEffect->InnerRadius);
	if (DamageRadius <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Projectile[%s]: detonation damage effect has no radius."), *GetNameSafe(this));
		return;
	}

	const FVector DamageOrigin = GetActorLocation();
	const float DamageRadiusSquared = FMath::Square(DamageRadius);
	for (TActorIterator<AAgent> AgentIt(World); AgentIt; ++AgentIt)
	{
		AAgent* HitAgent = *AgentIt;
		if (!HitAgent || HitAgent == GetOwner() || HitAgent == GetInstigator())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(DamageOrigin, HitAgent->GetActorLocation());
		if (DistanceSquared > DamageRadiusSquared)
		{
			continue;
		}

		const float DamageAmount = DamageEffect->GetDamageAmountAtDistance(FMath::Sqrt(DistanceSquared));
		if (DamageAmount > 0.0f)
		{
			HitAgent->ApplyDamage(DamageAmount, this);
		}
	}
}

/**
 * @brief Handles valid projectile impacts against world actors.
 *
 * @param HitComponent Component on this projectile that generated the hit.
 * @param OtherActor Actor hit by the projectile.
 * @param OtherComponent Component hit on the other actor.
 * @param NormalImpulse Physics impulse generated by the hit.
 * @param Hit Detailed impact data including impact point and normal.
 */
void AProjectile::HandleHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	if (DetonationTrigger != EProjectileDetonationTrigger::Timed)
	{
		if (AAgent* HitAgent = Cast<AAgent>(OtherActor))
		{
			const float DamageAmount = DamageEffect ? DamageEffect->GetDamageAmount() : 0.0f;
			HitAgent->ApplyDamage(DamageAmount, this);
		}
	}

	if (DetonationTrigger == EProjectileDetonationTrigger::OnImpact)
	{
		Detonate();
		return;
	}

	switch (ImpactBehavior)
	{
	case EProjectileImpactBehavior::Bounce:
		return;
	case EProjectileImpactBehavior::Penetrate:
		return;
	case EProjectileImpactBehavior::Stick:
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->StopMovementImmediately();
			ProjectileMovementComponent->Deactivate();
		}
		if (ImpactVFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				this,
				ImpactVFX,
				Hit.ImpactPoint,
				Hit.ImpactNormal.Rotation());
		}
		if (OtherComponent)
		{
			AttachToComponent(OtherComponent, FAttachmentTransformRules::KeepWorldTransform);
		}
		return;
	case EProjectileImpactBehavior::Detonate:
		Detonate();
		return;
	case EProjectileImpactBehavior::DestroyOnImpact:
	default:
		if (ImpactVFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				this,
				ImpactVFX,
				Hit.ImpactPoint,
				Hit.ImpactNormal.Rotation());
		}
		Destroy();
		return;
	}
}
