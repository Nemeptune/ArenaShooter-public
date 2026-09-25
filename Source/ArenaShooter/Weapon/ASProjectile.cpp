// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ASProjectile.h"

#include "System/ASGameplayTags.h"
#include "System/ASLogChannels.h"
#include "Player/ASPlayerController.h"
#include "GameplayEffect.h"
#include "Physics/KnockbackStatics.h"
#include "NiagaraComponent.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "AbilitySystem/ASAbilitySystemGlobals.h"
#include "Components/ArrowComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Settings/GASDeveloperSettings.h"
#include "System/ASProfiling.h"

AASProjectile::AASProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComp->SetCollisionProfileName("Projectile");
	CollisionComp->CanCharacterStepUpOn = ECB_No;
	CollisionComp->bTraceComplexOnMove = true;
	CollisionComp->bReceivesDecals = false;
	SetRootComponent(CollisionComp);

	HitboxComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitboxComponent"));
	HitboxComp->SetCollisionProfileName("MeshHitDetection"); 
	HitboxComp->CanCharacterStepUpOn = ECB_No;
	HitboxComp->OnComponentBeginOverlap.AddDynamic(this, &AASProjectile::OnHitBoxOverlapBegin);
	HitboxComp->bTraceComplexOnMove = true;
	HitboxComp->SetUseCCD(true);
	HitboxComp->bReceivesDecals = false;
	HitboxComp->SetupAttachment(RootComponent);
	HitboxComp->SetShouldUpdatePhysicsVolume(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionComp);
	ProjectileMovement->bForceSubStepping = true;
	ProjectileMovement->MaxSimulationTimeStep = 0.0166f;
	ProjectileMovement->MaxSimulationIterations = 16;
	ProjectileMovement->bInterpolationUseScopedMovement = false;
	ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AASProjectile::OnBounce);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AASProjectile::OnStop);

#if WITH_EDITORONLY_DATA
	ArrowComp = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	if (ArrowComp)
	{
		ArrowComp->ArrowColor = FColor(150,200,255);
		ArrowComp->bTreatAsASprite = true;
		ArrowComp->SetupAttachment(CollisionComp);
		ArrowComp->bIsScreenSizeScaled = true;
		ArrowComp->SetSimulatePhysics(false);
	}
#endif

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	InitialLifeSpan = 5.0f;

	//Networking
	bReplicates = true;
	bReplicateProjectileMovement = false;
	NetPriority = 2.0f;
	SetMinNetUpdateFrequency(100.f);

	bHasSpawnedFully = false;
	ProjectileId =	NULL_PROJECTILE_ID;
	bIsFakeProjectile = false;
	LinkedFakeProjectile = nullptr;
	LinkedAuthProjectile = nullptr;
	bCorrectFakeProjectilePositionOverTime = false;
	InitialProjectileError = 0.0f;
	bLimitBounces = false;
	MaximumBounces = 1;
	bStraightenProjectileOnBounce = true;
	bDetonated = false;
	bInOverlap = false;
	bFinishedResim = false;
	ImpactGameplayEffect = nullptr;
	bUseFilter = true;
	//Filter.TeamFilter = ETargetDataFilterTeam::TDFT_NoTeammates;
	Filter.SelfFilter = ETargetDataFilterSelf::TDFS_NoSelf;
	ImpactEffectDirection = EImpactEffectDirection::InProjectileDirection;
	AreaRadius = 0.0f;
	AreaOffset = FVector(0.0f);
	AreaGameplayEffect = nullptr;
	bSkipAreaEffectForImpactTarget = false;
	LifeSpanAfterDetonation = 0.0f;
	bDetonateOnLifeSpanEnd = false;
	bPredictFX = true;
	BounceVelocityFXThreshold = 100.0f;
	bTriggeredFX = false;
	ASPlayerController = nullptr;
	bHasBounced = false;
	BounceCount = 0;
	TearOffDelay = 0.1f;
	MinLifetime = 0.05f;
}

void AASProjectile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SpawnTransform = Transform;
}

void AASProjectile::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	if (GetInstigator())
	{
		ASPlayerController = GetInstigatorController<AASPlayerController>();

		Filter.SelfActor = GetInstigator();
	}

	if (ProjectileMovement->bShouldBounce && bStraightenProjectileOnBounce)
	{
		if (ASPlayerController.IsValid())
		{
			InitialOwnerAimRotation = ASPlayerController->GetControlRotation();
			InitialOwnerAimRotation.Normalize();
		}
		else if (GetInstigator())
		{
			// Disable correction
			PROJECTILE_LOG(Error, TEXT("Projectile (%s) wants to correct to its owner's aim rotation on bounce (bStraightenProjectileOnBounce enabled), but couldn't find control rotation for instigator (%s). Projectile's direction will not be corrected."), *GetName(), *GetNameSafe(GetInstigator()));
			bHasBounced = true;
		}
	}
	
	TArray<UMeshComponent*> MeshComponents;
	GetComponents<UMeshComponent>(MeshComponents);
	for (auto MeshComp : MeshComponents)
	{
		MeshComp->bUseAsOccluder = false;
		MeshComp->SetCastShadow(false);
	}
}

void AASProjectile::BeginPlay()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AASProjectile::BeginPlay);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_Projectiles);
	
	Super::BeginPlay();
	
	ASProfiling::AddToGauge(TEXT("ProjectileActors"), 1);

	if ((!HasAuthority() && ProjectileId != NULL_PROJECTILE_ID) || bIsFakeProjectile)
	{
		if (!ensureAlwaysMsgf(GetInstigator() && ASPlayerController.IsValid(), TEXT("Instigating player controller could not be found for predicted projectile projectile (%s). Failed to find player controller with instigator (%s). Predicted projectiles must be spawned with an instigator with a valid player controller."),
		*GetName(), *GetNameSafe(GetInstigator())))
		{
			Destroy();
			return;
		}	
	}

	if (GetInstigator())
	{
		if (!ensureAlwaysMsgf(IsValid(Owner), TEXT("Projectile spawned by (%s) does not have a valid owner. Projectiles spawned by players or AI must have an owner so their ASC can be found to apply gameplay effects. Otherwise, GEs will be applied without an instigator."), *GetInstigator()->GetName()))
		{
			Destroy();
			return;
		}
	}

	if (GetInstigator())
	{
		Filter.SelfActor = GetInstigator();
	}

	if (!ensureAlwaysMsgf(!(GetNetMode() == NM_Client && HasAuthority() && !bIsFakeProjectile), TEXT("Spawned projectile (%s) directly on client, which is not allowed. Projectiles should be spawned on the server to be replicated to clients, or should be spawned predictively with the \"Spawn Predicted Projectile\" ability task."), *GetName()))
	{
		Destroy();
		return;
	}

	if (!HasAuthority() && ProjectileId != NULL_PROJECTILE_ID)
	{
		float CatchupTickDelta = (ASPlayerController->PlayerState) ? (0.0005f * ASPlayerController->PlayerState->ExactPing) : 0.0f;
		if (CatchupTickDelta > 0.0f && bCorrectFakeProjectilePositionOverTime)
		{
			CatchupTick(CatchupTickDelta);
		}
		
		PROJECTILE_LOG(Verbose, TEXT("(%i:%i:%i) (ID: %i): Successfully replicated authoritative projectile (%s) to client."),
			FDateTime::UtcNow().GetMinute(), FDateTime::UtcNow().GetSecond(), FDateTime::UtcNow().GetMillisecond(), ProjectileId, *GetName());

		if (ensureAlwaysMsgf(ASPlayerController->FakeProjectiles.Contains(ProjectileId),
			TEXT("Client-side authoritative projectile (%s) failed to find corresponding fake projectile with ID (%i)."),
			*GetNameSafe(this), ProjectileId))
		{
			LinkFakeProjectile(ASPlayerController->FakeProjectiles[ProjectileId]);
			ASPlayerController->FakeProjectiles.Remove(ProjectileId);
		}
	}

	if ((GetLocalRole() == ROLE_SimulatedProxy) && (ProjectileId == NULL_PROJECTILE_ID))
	{
		if (!bFinishedResim && !GetWorldTimerManager().IsTimerActive((FinishedResimulationTimer)))
		{
			SetActorTransform(SpawnTransform);
		}
	}

#if WITH_EDITOR
	if (const UGASDeveloperSettings* DevSettings = GetDefault<UGASDeveloperSettings>())
	{
		if (DevSettings->ProjectileDebugMode != EProjectileDebugMode::None)
		{
			if (HasAuthority() && !bIsFakeProjectile)
			{
				const float ServerDrawScale = (20.0f / DevSettings->DrawFrequency);
				constexpr float MinRate = 0.01f;
				const float DrawRate = FMath::Max((ServerDrawScale / ProjectileMovement->InitialSpeed), MinRate);
				GetWorldTimerManager().SetTimer(DrawDebugTimer, FTimerDelegate::CreateUObject(this, &AASProjectile::DrawDebug), DrawRate, true);
			}
			else
			{
				const float ClientDrawScale = (100.0f / DevSettings->DrawFrequency);
				constexpr float MinRate = 0.02f;
				const float DrawRate = FMath::Max((ClientDrawScale / ProjectileMovement->InitialSpeed), MinRate);
				if (IsValid(LinkedFakeProjectile))
				{
					GetWorldTimerManager().SetTimer(DrawDebugTimer, FTimerDelegate::CreateUObject(this, &AASProjectile::DrawDebug), DrawRate, true);
					GetWorldTimerManager().ClearTimer(LinkedFakeProjectile->DrawDebugTimer);
				}
				else if (bIsFakeProjectile && !DevSettings->bWaitForLinkage)
				{
					GetWorldTimerManager().SetTimer(DrawDebugTimer, FTimerDelegate::CreateUObject(this, &AASProjectile::DrawDebug), DrawRate, true);
				}
			}
		}

		if (DevSettings->bDrawSpawnPosition)
		{
			DrawProjectileStep(GetDebugColor(), false, true);
		}
	}
#endif

	if (!IsValid(LinkedFakeProjectile))
	{
		StartFlightLoop();
	}
	
	bHasSpawnedFully = true;
}

void AASProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ASProfiling::AddToGauge(TEXT("ProjectileActors"), -1);
	Super::EndPlay(EndPlayReason);
}

void AASProjectile::CatchupTick(float CatchupTickDelta)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AASProjectile::CatchupTick);
	
	if (ProjectileMovement)
	{
		ProjectileMovement->TickComponent(CatchupTickDelta, LEVELTICK_All, nullptr);
	}
}

void AASProjectile::SetLifeSpan(float InLifeSpan)
{
	InitialLifeSpan = InLifeSpan;
	if (InLifeSpan > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_LifeSpanExpired, this, &AActor::LifeSpanExpired, InLifeSpan);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_LifeSpanExpired);
	}
}

void AASProjectile::TornOff()
{
	/* The projectile is torn off when it detonates on the server, which forces detonation on clients if it hasn't
	* already. */
	if (!bDetonated)
	{
		if (DetonationInfo.bDetonated)
		{
			// Server tore off a non-owning simulated proxy before it finished its local resimulation.
			if (ProjectileId == NULL_PROJECTILE_ID)
			{
				// BUG: Figure out why this is being called on every simulated proxy every time. Unless we detonated on
				// spawn, aren't we SUPPOSED to tear off before resimulation and wait for the resim timer before
				// detonating? It doesn't seem to be causing any problems regardless, probably because projectiles that
				// haven't finished resim have their detonation rejected in Detonate.
				// PROJECTILE_LOG(Warning, TEXT("Projectile (%s) on non-owning simulated proxy was torn off before it finished its resimulation. Is MinLifetime less than TearOffDelay? Detonating early with replicated detonation information..."), *GetName());
				DetonateWithDetonationInfo();
				// looks like we call DetonateWithDetonationInfo in every case, thats why
			}
			/*
			 * Predicting client's version of the authoritative projectile hasn't detonated yet. This happens when the
			 * projectile detonates on spawn, but the detonation is rejected because the projectile hasn't been fully
			 * initialized yet (@see AProjectile::Detonate), in which case, we try to detonate again once being torn
			 * off.
			 *
			 * This can also theoretically happen if the client's version of the authoritative projectile somehow misses
			 * whatever it hit on the server, though this is extremely rare.
			 */
			else
			{
				DetonateWithDetonationInfo();
			}
		}
		// Server tore off a non-owning simulated proxy without replicating the detonation.
		else if (ProjectileId == NULL_PROJECTILE_ID)
		{
			checkf(1, TEXT("Projectile (%s) on non-owning simulated proxy was torn off, but it never received any detonation information, meaning the server tore off the projectile without detonating it."), *GetName());
		}
	}
}

void AASProjectile::LifeSpanExpired()
{
	/* If we want to detonate this projectile when its lifespan ends, and it hasn't detonated yet, force its detonation.
	 * This will eventually lead to this actor's destruction when ShutDown updates LifeSpan (to ensure enough time for
	 * replication), which will call this function again, leading to its other branch since bDetonated will now be
	 * true. */
	if (bDetonateOnLifeSpanEnd && !bDetonated)
	{
		const FHitResult Hit = FHitResult(nullptr, nullptr, CollisionComp->GetComponentLocation(), CollisionComp->GetComponentRotation().Vector());
		OnStop(Hit);	
	}
	else
	{
		Super::LifeSpanExpired();	
	}
}

void AASProjectile::ShutDown()
{
	StopFlightLoop();
	
#if WITH_EDITOR
	GetWorldTimerManager().ClearTimer(DrawDebugTimer);
#endif

	/* If we want to keep this projectile visible after its detonation (i.e. for post-detonation FX, like a landing
	 * animation), make sure it's not hidden. E.g. if we're not predicting FX, we need to unhide the authoritative
	 * projectile when it detonates, since authoritative projectiles are hidden by default on the owning client. */
	if (bTriggeredFX && (LifeSpanAfterDetonation > 0.0f))
	{
		SetActorHiddenInGame(false);
		TArray<USceneComponent*> Components;
		GetComponents<USceneComponent>(Components);
		for (int32 i = 0; i < Components.Num(); i++)
		{
			Components[i]->SetVisibility(true);
		}
	}

	if (!IsPendingKillPending())
	{
		SetActorEnableCollision(false);
		ProjectileMovement->SetActive(false);

		// Make sure we keep the projectile alive long enough to finish replicating.
		float NewLifeSpan = FMath::Max(LifeSpanAfterDetonation, 0.4f);

		if (LifeSpanAfterDetonation == 0.0f)
		{
			DisableAndHide();
		}
		/* If the fake projectile isn't predicting FX, it should always be disabled and hidden after detonation. We'll
		 * be switching to the authoritative projectile for FX and post-detonation effects instead. We still want to
		 * keep the fake projectile alive for a bit, so the authoritative projectile has opportunity to check for missed
		 * predictions. */
		else if (bIsFakeProjectile && !bPredictFX)
		{
			DisableAndHide();
		}
		else if (LifeSpanAfterDetonation < NewLifeSpan)
		{
			GetWorldTimerManager().SetTimer(ShutDownTimer, FTimerDelegate::CreateUObject(this, &AASProjectile::DisableAndHide), LifeSpanAfterDetonation, false);
		}

		SetLifeSpan(NewLifeSpan);

		K2_ShutDown();
	}
}

void AASProjectile::Destroyed()
{
#if WITH_EDITOR
	/* If this projectile never detonated, draw its final position if desired, since it won't get a chance to in
	 * Detonate(). This usually happens if the fake projectile misses its target and gets replaced by the authoritative
	 * projectile. */
	if (!bDetonated)
	{
		if (const UGASDeveloperSettings* DevSettings = GetDefault<UGASDeveloperSettings>())
		{
			if (DevSettings->bDrawFinalPosition && GetWorld()->IsGameWorld())
			{
				DrawProjectileStep(GetDebugColor(), false, true);
			}
		}
	}
#endif

	// Redundancy for safety. Should never happen.
	if (LinkedFakeProjectile)
	{
		LinkedFakeProjectile->Destroy();
	}

	GetWorldTimerManager().ClearAllTimersForObject(this);
	
	StopFlightLoop();
	
	Super::Destroyed();
}

void AASProjectile::DisableAndHide()
{
	ForEachComponent(false, [](UActorComponent* InComponent)
	{
		if (UNiagaraComponent* NiagaraComp = Cast<UNiagaraComponent>(InComponent))
		{
			NiagaraComp->SetForceSolo(true);
			NiagaraComp->TickComponent(0.0f, LEVELTICK_All, nullptr);
			NiagaraComp->Deactivate();
			NiagaraComp->SetAutoDestroy(true);
		}
		else if (UAudioComponent* AudioComp = Cast<UAudioComponent>(InComponent))
		{
			if (AudioComp->GetSound() != nullptr && !AudioComp->GetSound()->IsOneShot())
			{
				AudioComp->Stop();
			}
		}
		else if (USceneComponent* SceneComp = Cast<USceneComponent>(InComponent))
		{
			SceneComp->SetHiddenInGame(true);
			SceneComp->SetVisibility(false);
		}
	});
}

void AASProjectile::StartFlightLoop()
{
	if (!FlightLoopSound || FlightAudioComp || bTriggeredFX)
	{
		return;
	}

	FlightAudioComp = UGameplayStatics::SpawnSoundAttached(FlightLoopSound, GetRootComponent(), NAME_None,
		FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget,
		true, 1.f, 1.f, 0.f, nullptr, nullptr, true);
}

void AASProjectile::StopFlightLoop()
{
	if (FlightAudioComp)
	{
		FlightAudioComp->FadeOut(FlightLoopFadeTime, 0.f);
		FlightAudioComp = nullptr;
	}
}

void AASProjectile::InitFakeProjectileId(AASPlayerController* OwningPlayer,uint32 InProjectileId)
{
	if (InProjectileId == NULL_PROJECTILE_ID)
	{
		return;
	}

	bIsFakeProjectile = true;
	if (OwningPlayer)
	{
		OwningPlayer->FakeProjectiles.Add(InProjectileId, this);
	}
}

void AASProjectile::LinkFakeProjectile(AASProjectile* InFakeProjectile)
{
	LinkedFakeProjectile = InFakeProjectile;
	InFakeProjectile->LinkedAuthProjectile = this;

	InitialProjectileError = (GetActorLocation() - LinkedFakeProjectile->GetActorLocation()).Size();
	if (((GetActorLocation() - LinkedFakeProjectile->GetActorLocation()) | LinkedFakeProjectile->GetVelocity()) > 0.f)
	{
		InitialProjectileError *= -1.0f;
	}

	PROJECTILE_LOG(VeryVerbose, TEXT("Fake projectile (%s) linked. Error Margin: (%f)."), *GetNameSafe(LinkedFakeProjectile), InitialProjectileError);

	SetActorHiddenInGame(true);
	TArray<USceneComponent*> Components;
	GetComponents<USceneComponent>(Components);
	for (int32 i = 0; i < Components.Num(); i++)
	{
		Components[i]->SetVisibility(false);
	}
}


void AASProjectile::SwitchToRealProjectile()
{
	SetActorHiddenInGame(false);
	TArray<USceneComponent*> Components;
	GetComponents<USceneComponent>(Components);
	for (int32 i = 0; i < Components.Num(); i++)
	{
		Components[i]->SetVisibility(true);
	}

	if (LinkedFakeProjectile)
	{
		LinkedFakeProjectile->Destroy();
		LinkedFakeProjectile = nullptr;
	}
	
	StartFlightLoop();
}

void AASProjectile::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AASProjectile::Tick);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_Projectiles);
	
	Super::Tick(DeltaTime);

	if (bCorrectFakeProjectilePositionOverTime)
	{
		CorrectionLerpTick(DeltaTime);
	}
}

void AASProjectile::CorrectionLerpTick(float DeltaTime)
{
	if (LinkedFakeProjectile && !LinkedFakeProjectile->bDetonated && !LinkedFakeProjectile->IsPendingKillPending() && ProjectileMovement)
	{
		if (DeltaTime > UE_SMALL_NUMBER)
		{
			FVector Current = LinkedFakeProjectile->GetActorLocation();
			FVector Target = GetActorLocation();

			/* We choose to lerp the projectile over a duration of (2000 / its initial speed). E.g. a projectile that
			 * travels at 1000m/s will be lerped over 2.0s. Decrease this duration if the projectile is using physics,
			 * since it has a higher risk of becoming desynced. I don't think this is something designers should worry
			 * about, but we could expose it for configuration if we want to. */
			// TODO: Update this to adjust lerp-rate based on target delta.
			const float TargetLerpTime = (ProjectileMovement->bShouldBounce ? 200.f : 2000.f) / ProjectileMovement->InitialSpeed;
			const float StepDistance = (InitialProjectileError / TargetLerpTime);

			FVector NewLoc = FMath::VInterpConstantTo(Current, Target, DeltaTime, StepDistance);

			FRepMovement RepMovement(LinkedFakeProjectile->GetReplicatedMovement());
			RepMovement.Location = NewLoc;
			RepMovement.Rotation = LinkedFakeProjectile->GetActorRotation();
			LinkedFakeProjectile->SetReplicatedMovement(RepMovement);
			LinkedFakeProjectile->PostNetReceiveLocationAndRotation();

#if WITH_EDITOR
			if (GetDefault<UGASDeveloperSettings>()->bLogCorrection)
			{
				PROJECTILE_LOG(Log, TEXT("(%f): Moved fake projectile (%fm). Currently (%fm) from target."),GetWorld()->GetTimeSeconds(), FVector::Dist(NewLoc, Current), FVector::Dist(Target, NewLoc));
			}
#endif
		}
	}
}

void AASProjectile::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (FMath::Abs(ImpactVelocity.Dot(ImpactResult.Normal)) > BounceVelocityFXThreshold)
	{
		// Trigger FX on owning client's auth projectile if we aren't predicting them.
		const bool bIsAuthOnOwner = ((!HasAuthority() || GetTearOff()) && (ProjectileId != NULL_PROJECTILE_ID));
		if (bIsAuthOnOwner)
		{
			/* Trigger FX on fake projectile if we ARE predicting them, or if we don't have an auth projectile yet, since
			 * our auth projectile probably won't finish initializing before it was supposed to bounce. */
			if (!bIsFakeProjectile || bPredictFX || !LinkedAuthProjectile)
			{
				const FRotator Rotation = FRotationMatrix::MakeFromXY(ImpactResult.ImpactNormal, FVector::RightVector).Rotator();
				BounceFX.ExecuteEffects(this, ImpactResult.ImpactPoint, Rotation, ImpactResult.GetComponent());
			}
		}
	}

	if (bStraightenProjectileOnBounce && !bHasBounced)
	{
		const FVector CurrentVelocity = ProjectileMovement->Velocity;
		const FVector Normal = ImpactResult.ImpactNormal;
		const FVector ProjectedVelocity = CurrentVelocity - ((CurrentVelocity | Normal) * Normal);
		const FVector InitialAimDir = InitialOwnerAimRotation.Vector();
		const FVector ProjectedAimDir = (InitialAimDir - (InitialAimDir | Normal) * Normal).GetSafeNormal();
		const FQuat AlignmentRotation = FQuat::FindBetweenNormals(ProjectedVelocity.GetSafeNormal(), ProjectedAimDir);
		const FVector TargetVec = AlignmentRotation.RotateVector(CurrentVelocity);
		ProjectileMovement->Velocity = TargetVec;
		bHasBounced = true;
	}

	BounceCount++;
	if (bLimitBounces && BounceCount == MaximumBounces)
	{
		ProjectileMovement->bShouldBounce = false;
	}
}

void AASProjectile::OnHitBoxOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AASProjectile::OnHitBoxOverlapBegin);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_Projectiles);
	
	if (!IsValid(OtherActor))
	{
		return;
	}

	if (OtherActor == GetInstigator())
	{
		return;
	}

	if (!bInOverlap)
	{
		TGuardValue<bool> OverlapGuard(bInOverlap, true);

		if (bUseFilter && !Filter.FilterPassesForActor(OtherActor))
		{
			return;
		}

		// Check for line of sight
		const FVector ImpactPoint = (bFromSweep ? FVector(SweepResult.ImpactPoint) : (OtherComp != nullptr ? OtherComp->GetComponentLocation() : OtherActor->GetActorLocation()));
		FCollisionQueryParams Params(FName(TEXT("HitboxOverlapTrace")), true, this);
		Params.AddIgnoredActor(OtherActor);
		if (GetWorld()->LineTraceTestByChannel(ImpactPoint, GetActorLocation(), ECC_Visibility, Params))
		{
			return;
		}

		FHitResult Hit;

		if (bFromSweep)
		{
			Hit = SweepResult;
		}
		else
		{
			// Try to generate a hit by sweeping the hitbox component.
			OtherComp->SweepComponent(Hit, GetActorLocation() - (GetVelocity() * 10.f), GetActorLocation() + GetVelocity(), HitboxComp->GetComponentQuat(), HitboxComp->GetCollisionShape(), HitboxComp->bTraceComplexOnMove);

			// Try to generate a hit by just tracing against the component.
			if (Hit.GetActor() != OtherActor)
			{
				OtherComp->LineTraceComponent(Hit, GetActorLocation() - (GetVelocity() * 10.f), GetActorLocation() + GetVelocity(), FCollisionQueryParams(GetClass()->GetFName(), false, this));
			}

			// If we STILL fail, we'll just construct the hit manually. This is fine, since it's just for visuals.
			if (Hit.GetActor() != OtherActor)
			{
				const FVector Normal = (ImpactPoint - GetActorLocation()).GetSafeNormal();
				Hit = FHitResult(OtherActor, OtherComp, ImpactPoint, Normal); 
			}
		}

		Detonate(true, OtherActor, OtherComp, Hit.Location, Hit.ImpactNormal);
	}
}

void AASProjectile::OnStop(const FHitResult& Hit)
{
	Detonate(false, Hit.GetActor(), Hit.GetComponent(), Hit.ImpactPoint, Hit.Normal);
}

void AASProjectile::FellOutOfWorld(const class UDamageType& dmgType)
{
	if (HasAuthority() || GetLocalRole() == ROLE_None)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(this,[this, &TimerHandle]()
		{
			GetWorldTimerManager().ClearTimer(TimerHandle);
			Destroy();
		}), InitialLifeSpan, false);
	}

	ShutDown();
}

void AASProjectile::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	if (bReplicateProjectileMovement && IsServerProjectile())
	{
		if (RootComponent && RootComponent->GetAttachParent())
		{
			Super::PreReplication(ChangedPropertyTracker);
		}
		else
		{
			GatherCurrentMovement();
		}
	}
}

void AASProjectile::GatherCurrentMovement()
{
	if (RootComponent)
	{
		if (RootComponent->GetAttachParent())
		{
			Super::GatherCurrentMovement();
		}
		else
		{
			ReplicatedProjectileMovement.Location = RootComponent->GetComponentLocation();
			ReplicatedProjectileMovement.Rotation = RootComponent->GetComponentRotation();
			ReplicatedProjectileMovement.LinearVelocity = GetVelocity();

			MARK_PROPERTY_DIRTY_FROM_NAME(AASProjectile, ReplicatedProjectileMovement, this);
		}
	}
}

void AASProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	ProjectileMovement->Velocity = NewVelocity;
}

FVector AASProjectile::GetVelocity() const
{
	if (RootComponent != nullptr && (RootComponent->IsSimulatingPhysics() || ProjectileMovement == nullptr))
	{
		return GetRootComponent()->GetComponentVelocity();
	}
	else
	{
		return ProjectileMovement->Velocity;
	}
}

void AASProjectile::OnRep_ReplicatedMovement()
{
	const FRepMovement& LocalRepMovement = GetReplicatedMovement();
	if (RootComponent)
	{
		/* Sync physics sim to match server. bRepPhysics should always be false for projectiles, so this shouldn't
		 * really be necessary. */
		if (RootComponent->IsSimulatingPhysics() != LocalRepMovement.bRepPhysics)
		{
			SyncReplicatedPhysicsSimulation();
		}

		RootComponent->OnReceiveReplicatedState(LocalRepMovement.Location, LocalRepMovement.Rotation.Quaternion(), LocalRepMovement.LinearVelocity, LocalRepMovement.AngularVelocity);

		if (!RootComponent->GetAttachParent())
		{
			if (GetLocalRole() == ROLE_SimulatedProxy)
			{
				if (ProjectileId != NULL_PROJECTILE_ID)
				{
					PostNetReceiveVelocity(LocalRepMovement.LinearVelocity);
					PostNetReceiveLocationAndRotation();
				}
			}
		}
	}
}

void AASProjectile::OnRep_RepProjectileMovement()
{
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		FRepMovement NewMovement(GetReplicatedMovement());
		NewMovement.Location = ReplicatedProjectileMovement.Location;
		NewMovement.Rotation = ReplicatedProjectileMovement.Rotation;
		NewMovement.LinearVelocity = ReplicatedProjectileMovement.LinearVelocity;
		NewMovement.AngularVelocity = FVector(0.f);
		NewMovement.bSimulatedPhysicSleep = false;
		NewMovement.bRepPhysics = false;
		SetReplicatedMovement(NewMovement);

		OnRep_ReplicatedMovement();
	}
}

void AASProjectile::Detonate(bool bHasDirectImpactTarget, AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AASProjectile::Detonate);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_Projectiles);
	
	/** Never detonate non-owning simulated proxies until they've finished their local resimulation. */
	if (!IsServerProjectile() && ProjectileId == NULL_PROJECTILE_ID && !bFinishedResim)
	{
		return;
	}
	
	/* Replicated authoritative projectiles aren't allowed to detonate before they finish initializing (i.e. on spawn),
	 * because we can't check for missed predictions yet. Instead, we wait until we get torn off, then try to detonate
	 * again. */
	if (!bDetonated && (HasAuthority() || bHasSpawnedFully))
	{
		PROJECTILE_LOG(VeryVerbose, TEXT("%s projectile (%s) hit actor (%s) on (%s). Direct hit? (%s)"),
			*FString(bIsFakeProjectile ? "Fake" : "Authoritative"), *GetName(), *GetNameSafe(OtherActor), *FString(IsServerProjectile() ? "server" : (ProjectileId == NULL_PROJECTILE_ID ? "remote client" : "owning client")), *LexToString(bHasDirectImpactTarget));

		/* Don't trigger FX with the owning client's authoritative projectile if the fake projectile is allowed to
		 * predict them, unless the fake projectile had a missed prediction that needs to be corrected. */
		const bool bIsAuthOnOwner = ((!HasAuthority() || GetTearOff()) && (ProjectileId != NULL_PROJECTILE_ID));
		if (!bIsAuthOnOwner || ShouldAuthProjDetonateToOwner(true))
		{
			// Don't trigger FX with fake projectiles if they don't predict them.
			if (!bIsFakeProjectile || bPredictFX)
			{
				bTriggeredFX = true;

				PROJECTILE_LOG(VeryVerbose, TEXT("\t\t... FX triggered."));

				const FVector Normal = (bHasDirectImpactTarget ? (GetActorLocation() - OtherActor->GetActorLocation()).GetSafeNormal() : HitNormal);

				DetonationFX.ExecuteEffects(this, HitLocation, Normal.Rotation(), OtherComp, bHasDirectImpactTarget);

				/* Spawn missed impact FX if we didn't directly hit a target (i.e. detonated against the environment),
				 * unless we didn't actually hit anything (i.e. detonated from our lifespan expiring). */
				// TODO: add MissedProjectileFX, for now we spawn DetonationFX
				if (!bHasDirectImpactTarget && (IsValid(OtherActor) || IsValid(OtherComp)))
				{
					const FRotator Rotation = FRotationMatrix::MakeFromXY(Normal, CollisionComp->GetComponentRotation().Vector()).Rotator();
					DetonationFX.ExecuteEffects(this, HitLocation, Rotation, OtherComp);
				}

				/* Forward event to BP. We only do this when FX are triggered to ensure each client calls this exactly
				 * once per projectile (with the exception of missed predictions). */
				OnDetonate(bHasDirectImpactTarget, OtherActor, OtherComp, HitLocation, Normal);
			}

			/* If our authoritative projectile doesn't detonate soon, that means we predicted wrong and hit
			 * something we shouldn't have. If this happens, switch to our authoritative projectile in an effort to
			 * reconcile. */
			if (bIsFakeProjectile)
			{
				/* TearOffDelay is used here because when authoritative projectiles detonate on spawn, their detonation
				 * on clients will be delayed by TearOffDelay to ensure they've been fully initialized and have had
				 * enough time to initially replicate. */
				GetWorldTimerManager().SetTimer(SwitchToAuthTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					if (LinkedAuthProjectile)
					{
						if (!LinkedAuthProjectile->bDetonated)
						{
							PROJECTILE_LOG(Verbose, TEXT("Missed prediction: Fake projectile (%s) detonated against something it shouldn't have. Reconciling by switching to real projectile..."), *GetNameSafe(this));
							CSV_CUSTOM_STAT(ArenaShooter, ProjectileMispredictions, 1, ECsvCustomStatOp::Accumulate);
							LinkedAuthProjectile->SwitchToRealProjectile();
						}
					}
				}), (0.001 * (ASPlayerController->PlayerState->ExactPing + 60.f)) + TearOffDelay, false);
			}
		}

		//Apply gameplay effects on the server
		if (IsServerProjectile())
		{
			if (ImpactGameplayEffect && bHasDirectImpactTarget && OtherActor)
			{
				const FVector Normal = (ImpactEffectDirection == EImpactEffectDirection::InProjectileDirection) ? GetActorRotation().Vector() :
						(ImpactEffectDirection == EImpactEffectDirection::InVelocityDirection) ? ProjectileMovement->Velocity.GetSafeNormal() :
																		 (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
				FHitResult Hit = FHitResult(OtherActor, OtherComp, HitLocation, Normal);
				Hit.bBlockingHit = true;
				Hit.TraceStart = OtherActor->GetActorLocation();
				ApplyEffectToTarget(true, OtherActor, Hit);
				const FVector DirectOrigin = OtherActor->GetActorLocation() - ProjectileMovement->Velocity.GetSafeNormal() * 100.f;
			}

			// AOE effect
			if ((AreaRadius > 0.0f) && AreaGameplayEffect)
			{
				TArray<const AActor*> HitActors;
				TArray<FOverlapResult> OverlapResults;
				FVector Origin = GetAreaOfEffectOrigin();
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(this);
				GetWorld()->OverlapMultiByChannel(
					OverlapResults,
					Origin,
					FQuat(),
					ECC_WorldDynamic,
					FCollisionShape::MakeSphere(AreaRadius),
					QueryParams
				);
				
				for(const FOverlapResult& Overlap : OverlapResults)
				{
					if (AActor* ItTarget = Overlap.GetActor())
					{
						if (HitActors.Contains(ItTarget))
						{
							continue;
						}

						if (bSkipAreaEffectForImpactTarget && (ItTarget == OtherActor))
						{
							continue;
						}

						const bool bIsInstigator = (ItTarget == GetInstigator());
						if (bIsInstigator && !bAffectInstigator)
						{
							continue;
						}
						
						if (!bIsInstigator && bUseFilter && !Filter.FilterPassesForActor(ItTarget))
						{
							continue;
						}

						// Check line if sight
						FVector ImpactPoint = Overlap.OverlapObjectHandle.GetLocation();

						if (!HasBlastLineOfSight(Origin, ItTarget, ImpactPoint))
						{
							continue;
						}

						FHitResult OverlapHit = FHitResult(ItTarget, Overlap.GetComponent(), ImpactPoint, (ItTarget->GetActorLocation() - Origin).GetSafeNormal());
						OverlapHit.bBlockingHit = true;
						OverlapHit.TraceStart = ItTarget == OtherActor ? ItTarget->GetActorLocation() : Origin;

						const float Distance = FVector::Dist(Origin, ItTarget->GetActorLocation());
						
						ApplyEffectToTarget(false, ItTarget, OverlapHit, GetRadialFalloff(Distance));

						UKnockbackStatics::ApplyKnockbackToActor(ItTarget, Origin, GetRadialFalloff(Distance), MomentumParams, bIsInstigator);
						HitActors.Add(ItTarget);
					}
				}
			}

			DetonationInfo = FDetonationInfo(true, bHasDirectImpactTarget, OtherActor, OtherComp, HitLocation, HitNormal);
			MARK_PROPERTY_DIRTY_FROM_NAME(AASProjectile, DetonationInfo, this);

			// Enable movement replication so the detonation is in the correct location for clients.
			bReplicateProjectileMovement = true;

			/** Tear off the server's projectile to trigger TornOff on all clients, which will force them to
			 * detonate if they haven't already. We're basically using this like an RPC that replicates the
			 * detonation event to clients, but waits until after they're replicated and initialized. Delay this if
			 * the projectile hasn't had enough time to initially replicate yet. */
			if (GetGameTimeSinceCreation() > TearOffDelay)
			{
				GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::TearOff));
			}
			else
			{
				GetWorldTimerManager().SetTimer(TearOffTimer, FTimerDelegate::CreateUObject(this, &ThisClass::TearOff), TearOffDelay, false);
			}
		}

		/* If the authoritative projectile triggered FX when it shouldn't have, that means there was a missed prediction
		 * that caused us to switch to the authoritative projectile. If this happens, we need to destroy the fake one
		 * (if it hasn't been destroyed already), since it's no longer valid for predicting. This is usually only 
		 * necessary when projectiles have post-detonation effects (i.e. LifeSpanAfterDetonation > 0). Otherwise, the
		 * fake projectile will likely have already been hidden or destroyed. */
		if (LinkedFakeProjectile && bTriggeredFX && bPredictFX)
		{
			PROJECTILE_LOG(VeryVerbose, TEXT("\t\t... Fake projectile (%s) destroyed due to missed prediction."), *LinkedFakeProjectile.GetName());

			LinkedFakeProjectile->Destroy();
			LinkedFakeProjectile = nullptr;
		}

		bDetonated = true;
		ShutDown();

	#if WITH_EDITOR
		if (GetDefault<UGASDeveloperSettings>()->bDrawFinalPosition)
		{
			DrawProjectileStep(GetDebugColor(), false, true);
			DrawDetonationInfo(HitLocation, (bHasDirectImpactTarget ? (GetActorLocation() - OtherActor->GetActorLocation()).GetSafeNormal() : HitNormal));
		}
	#endif
	}
}

bool AASProjectile::ShouldAuthProjDetonateToOwner(bool bLog) const
{
	// If we aren't predicting FX, the authoritative projectile should trigger FX instead of the fake projectile.
	if (!bPredictFX)
	{
		return true;
	}

	if (LinkedFakeProjectile)
	{
		/*
		* If the fake projectile hasn't detonated yet, we should destroy it (which is done at the end of Detonate), and
		* switch to the authoritative projectile. This can occur if the fake projectile missed the correct target, or
		* if the authoritative projectile was slightly ahead of the fake one, because we forward-predicted a little too
		* far due to an inaccurate ping estimate.
		*/
		if (!LinkedFakeProjectile->bDetonated)
		{
			if (bLog)
			{
				PROJECTILE_LOG(Verbose, TEXT("Missed prediction: Fake projectile (%s) missed its detonation. Reconciling by destroying the fake projectile and using real projectile's detonation..."), *GetNameSafe(LinkedFakeProjectile));
				CSV_CUSTOM_STAT(ArenaShooter, ProjectileMispredictions, 1, ECsvCustomStatOp::Accumulate);
			}

			return true;
		}
		
		/* If the fake projectile already detonated, but did so inaccurately, we should correct it by replaying FX
		 * in the correct place with the authoritative projectile. */
		if (LinkedFakeProjectile->bDetonated)
		{
			constexpr float MaxFinalPositionDistance = 100.f; // Centimeters
			if ((LinkedFakeProjectile->GetActorLocation() - GetActorLocation()).Size() > MaxFinalPositionDistance)
			{
				if (bLog)
				{
					PROJECTILE_LOG(Verbose, TEXT("Missed prediction: Fake projectile (%s) detonated too early and/or in the wrong location (Error Margin: %fm). Reconciling by resimulating the detonation with the real projectile..."), *GetNameSafe(LinkedFakeProjectile), FVector::Dist(LinkedFakeProjectile->GetActorLocation(), GetActorLocation()) * 0.01f);
					CSV_CUSTOM_STAT(ArenaShooter, ProjectileMispredictions, 1, ECsvCustomStatOp::Accumulate);
				}

				return true;
			}
		}
	}
	/*
	 * If we don't have a fake projectile at this point, that means either the fake projectile detonated so early
	 * that it's already been destroyed, or the authoritative projectile detonated so early that we haven't linked
	 * yet.
	 *
	 * For the first case, if we've already been fully initialized and still don't have a linked projectile, that
	 * means it detonated so long ago that it was already destroyed. The fake projectile will have already caught
	 * this (using the SwitchToAuthTimer timer) and switched to using this authoritative projectile on the client.
	 * All we have to do at this point is replay the detonation with the real projectile.
	 * 
	 * The latter case is handled by TornOff. We don't allow replicated projectiles to detonate if they're not fully
	 * spawned. Instead, we wait until they get torn off (which will always occur AFTER BeginPlay), then try to
	 * detonate them again. This second detonation should execute successfully, and will either result in a
	 * successful prediction (the owning client's authoritative projectile, which now has a linked fake projectile,
	 * detonated with sufficient accuracy to that fake projectile; i.e. the branch above) or a missed prediction
	 * that will be caught by one of the above branches.
	 */
	if (!LinkedFakeProjectile)
	{
		if (bHasSpawnedFully)
		{
			if (bLog)
			{
				PROJECTILE_LOG(Verbose, TEXT("Missed prediction: Fake projectile detonated so early that it's already been destroyed and can't be found "
								 "(corresponding auth projectile: (%s)). The fake projectile should have switched to the real one by now. "
								 "Reconciling by using real projectile's detonation..."), *GetNameSafe(this));
				CSV_CUSTOM_STAT(ArenaShooter, ProjectileMispredictions, 1, ECsvCustomStatOp::Accumulate);
			}

			return true;
		}
	}

	return false;
}

const FMomentumParams& AASProjectile::GetMomentumParams() const
{
	return MomentumParams;
}

FGameplayEffectSpecHandle AASProjectile::MakeEffectSpec_Implementation(bool bDirectImpact, const AActor* Target, const FHitResult& Hit) const
{
	if (UASAbilitySystemComponent* TargetASC = UASAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
	{
		if (UASAbilitySystemComponent* InstigatorASC = UASAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
		{
			return InstigatorASC->MakeOutgoingSpecWithHitResult(bDirectImpact ? ImpactGameplayEffect : AreaGameplayEffect, 1.0f, Hit, this);
		}
		else
		{
			return TargetASC->MakeOutgoingSpecWithHitResult(bDirectImpact ? ImpactGameplayEffect : AreaGameplayEffect, 1.0f, Hit, this);
		}
	}

	return FGameplayEffectSpecHandle();
}

FVector AASProjectile::GetAreaOfEffectOrigin() const
{
	// XY in local space; Z in world space.
	const FVector LocalOffset2D = AreaOffset * FVector(1.f, 1.f, 0.f);
	const FVector WorldOffset2D = CollisionComp->GetComponentRotation().RotateVector(LocalOffset2D);
	const FVector FinalOffset = WorldOffset2D + FVector(0.f,0.f, AreaOffset.Z);
	return (CollisionComp->GetComponentLocation() + FinalOffset);
}

float AASProjectile::GetRadialFalloff(float Distance) const
{
	if (AreaRadius <= 0.f)
	{
		return 0.f;
	}
	if (Distance <= AreaInnerRadius)
	{
		return 1.f;
	}
	if (Distance >= AreaRadius)
	{
		return 0.f;
	}

	return 1.f - ((Distance - AreaInnerRadius) / FMath::Max(AreaRadius - AreaInnerRadius, UE_KINDA_SMALL_NUMBER));
}

bool AASProjectile::HasBlastLineOfSight(const FVector& Origin, const AActor* Target, const FVector& TargetPoint) const
{
	if (FVector::Dist(Origin, TargetPoint) <= CollisionTraceSkipRadius)
	{
		return true;
	}

	TArray<FVector, TInlineAllocator<3>> Origins;
	Origins.Add(Origin);
	Origins.Add(Origin + FVector(0.f, 0.f, BlastOriginLift));
	if (ProjectileMovement && !ProjectileMovement->Velocity.IsNearlyZero())
	{
		Origins.Add(Origin - ProjectileMovement->Velocity.GetSafeNormal() * BlastOriginLift);
	}

	FCollisionQueryParams Params(FName(TEXT("BlastLOS")), true, this);
	Params.AddIgnoredActor(Target);

	for (const FVector& TestOrigin : Origins)
	{
		if (!GetWorld()->LineTraceTestByChannel(TargetPoint, TestOrigin, ECC_Visibility, Params))
		{
			return true;
		}
	}

	return false;
}

void AASProjectile::ApplyEffectToTarget(const bool bDirectImpact, const AActor* Target, const FHitResult& Hit, float DamageScale) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AASProjectile::ApplyEffectToTarget);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_Damage);
	
	if (UASAbilitySystemComponent* TargetASC = UASAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
	{
		FGameplayEffectSpecHandle EffectSpec = MakeEffectSpec(bDirectImpact, Target, Hit);

		if (ensure(EffectSpec.IsValid()))
		{
			const float Base = (bDirectImpact ? ImpactDamage : AreaDamage) * DamageScale;
			EffectSpec.Data->SetSetByCallerMagnitude(FASGameplayTags::Data_Damage, Base);
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
		}
	}
}

void AASProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	constexpr bool bUsePushModel = true;

	FDoRepLifetimeParams OwnerOnlyParams{COND_OwnerOnly, REPNOTIFY_Always, bUsePushModel};
	DOREPLIFETIME_WITH_PARAMS_FAST(AASProjectile, ProjectileId, OwnerOnlyParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AASProjectile, ASPlayerController, OwnerOnlyParams);

	FDoRepLifetimeParams ReplicatedMovementParams{COND_SimulatedOrPhysics, REPNOTIFY_OnChanged, bUsePushModel};
	DOREPLIFETIME_WITH_PARAMS_FAST(AASProjectile, ReplicatedProjectileMovement, ReplicatedMovementParams);
	
	FDoRepLifetimeParams SimulatedOnlyParams{COND_SimulatedOnly, REPNOTIFY_OnChanged, bUsePushModel};
	DOREPLIFETIME_WITH_PARAMS_FAST(AASProjectile, SpawnTransform, SimulatedOnlyParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AASProjectile, DetonationInfo, SimulatedOnlyParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AASProjectile, InitialOwnerAimRotation, SimulatedOnlyParams);
}

void AASProjectile::GetReplicatedCustomConditionState(FCustomPropertyConditionState& OutActiveState) const
{
	Super::GetReplicatedCustomConditionState(OutActiveState);

	DOREPCUSTOMCONDITION_ACTIVE_FAST(AActor, AttachmentReplication, bReplicateProjectileMovement);
}

void AASProjectile::OnRep_DetonationInfo()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AASProjectile::OnRep_DetonationInfo);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_Projectiles);
	
	if (!ensure(DetonationInfo.bDetonated))
	{
		return;
	}

	if (ProjectileId == NULL_PROJECTILE_ID)
	{
		float RemainingDistance = (DetonationInfo.HitLocation - GetActorLocation()).Length();
		const float CurrentSpeed = (GetVelocity().IsZero() ? ProjectileMovement->InitialSpeed : GetVelocity().Length());
		float RemainingTime = RemainingDistance / CurrentSpeed;

		/* If this projectile detonated on spawn, rewind and resimulate it at a modified velocity to ensure it never
		 * detonates without ever being seen by other players. */
		if (!bHasSpawnedFully)
		{
			// Rewind the projectile now (instead of waiting for BeginPlay).
			SetActorTransform(SpawnTransform);

			// Modify this projectile's velocity to ensure it stays visible for at least MinLifeTime.
			RemainingDistance = (DetonationInfo.HitLocation - SpawnTransform.GetLocation()).Length();
			RemainingTime = FMath::Max((RemainingDistance / CurrentSpeed), MinLifetime);
			const float DesiredSpeed = RemainingDistance / RemainingTime;
			const float SpeedQuotient = DesiredSpeed / CurrentSpeed;

			// If we detonated on spawn, we might not have initialized our velocity yet.
			if (GetVelocity().IsZero())
			{
				ProjectileMovement->InitialSpeed *= SpeedQuotient;
			}
			else
			{
				ProjectileMovement->Velocity *= SpeedQuotient;
			}

			PROJECTILE_LOG(VeryVerbose, TEXT("Projectile (%s) detonated on spawn on non-owning simulated proxy. Rewinding and resimulating with a speed of (%f) for (%f) seconds."), *GetName(), DesiredSpeed, RemainingTime);
		}
		/* If this projectile detonated very close to its spawn, slow its velocity to ensure it stays visible long
		 * enough to be seen by players. */
		else if (GetGameTimeSinceCreation() + RemainingTime < MinLifetime)
		{
			RemainingTime = (MinLifetime - GetGameTimeSinceCreation());

			const float DesiredSpeed = RemainingDistance / RemainingTime;
			const float SpeedQuotient = DesiredSpeed / CurrentSpeed;

			ProjectileMovement->Velocity *= SpeedQuotient;

			PROJECTILE_LOG(VeryVerbose, TEXT("Projectile (%s) detonated before living for MinLifetime (%f) on non-owning simulated proxy. Finishing simulation at a speed of (%f) for (%f) seconds to make sure it lives long enough."), *GetName(), MinLifetime, DesiredSpeed, RemainingTime);
		}
		else
		{
			PROJECTILE_LOG(VeryVerbose, TEXT("Projectile (%s) on non-owning simulated proxy will stay alive for (%f) at its current speed of (%f) to finish its resimulation at a distance of (%f)."), *GetName(), RemainingTime, CurrentSpeed, RemainingDistance);
		}
		
		// Account for processing time this frame.
		RemainingTime -= GetWorld()->GetDeltaSeconds();
		
		auto DetonateDelayedFunc = [this]()
		{
			/* Make sure we detonate in the correct final location, since we skip OnRep_ReplicatedMovement on non-owning
			 * simulated proxies. */
			if (RootComponent && !RootComponent->GetAttachParent())
			{
				PostNetReceiveVelocity(GetReplicatedMovement().LinearVelocity);
				PostNetReceiveLocationAndRotation();
			}

			bFinishedResim = true;
			DetonateWithDetonationInfo();
		};

		/* If the time left in our resimulation is so short that it will finish before the next frame, skip the timer
		 * and detonate immediately. */
		if (RemainingTime > GetWorld()->GetDeltaSeconds())
		{
			GetWorldTimerManager().SetTimer(FinishedResimulationTimer, FTimerDelegate::CreateWeakLambda(this, DetonateDelayedFunc), RemainingTime, false);
		}
		else
		{
			GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, DetonateDelayedFunc));
		}
	}
}

void AASProjectile::DetonateWithDetonationInfo()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AASProjectile::DetonateWithDetonationInfo);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_Projectiles);
	
	if (!ensure(DetonationInfo.bDetonated))
	{
		return;
	}

	/* We do some data validation because this is usually called shortly after detonation info has been replicated, so
	 * there may be rare situations where the pointers are no longer valid. */
	Detonate
	(
		DetonationInfo.bHasDirectImpactTarget && IsValid(DetonationInfo.OtherActor),
		IsValid(DetonationInfo.OtherActor) ? DetonationInfo.OtherActor : nullptr,
		IsValid(DetonationInfo.OtherComp) ? DetonationInfo.OtherComp : nullptr,
		DetonationInfo.HitLocation,
		DetonationInfo.HitNormal
	);
}

bool AASProjectile::IsServerProjectile() const
{
	return (GetWorld()->GetNetMode() != NM_Client);
}

#if WITH_EDITOR
void AASProjectile::DrawDebug()
{
	if (!IsPendingKillPending())
	{
		if (const UGASDeveloperSettings* DevSettings = GetDefault<UGASDeveloperSettings>())
		{
			// Draw the server's authoritative projectile.
			if (HasAuthority() && !bIsFakeProjectile &&
				DevSettings->ProjectileDebugMode > EProjectileDebugMode::PredictedVersusClient)
			{
				DrawDebugSphere(GetWorld(), GetActorLocation(), CollisionComp->GetScaledSphereRadius() * 1.5f, 8, DevSettings->ServerProjectileColor, false, DevSettings->DrawTime);
			}
			// Draw the authoritative and/or fake projectile once they're linked.
			else if (!HasAuthority() && DevSettings->ProjectileDebugMode > EProjectileDebugMode::None)
			{
				// If the projectiles are synced, only make one draw to make this clear.
				if ((DevSettings->ProjectileDebugMode == EProjectileDebugMode::PredictedVersusClient || DevSettings->ProjectileDebugMode == EProjectileDebugMode::All) &&
					IsValid(LinkedFakeProjectile) && (GetActorLocation() - LinkedFakeProjectile->GetActorLocation()).Size() < 0.05f)
				{
					DrawProjectileStep(DevSettings->SyncedColor, true);
				}
				else
				{
					// Draw the client's authoritative projectile.
					if (DevSettings->ProjectileDebugMode > EProjectileDebugMode::None) // TODO: redundant check, the same check allows us to be in this scope
					{
						DrawProjectileStep(DevSettings->ClientAuthoritativeProjectileColor, true);
					}

					// Draw the client's fake projectile.
					if (IsValid(LinkedFakeProjectile) &&
						(DevSettings->ProjectileDebugMode == EProjectileDebugMode::PredictedVersusClient || DevSettings->ProjectileDebugMode == EProjectileDebugMode::All))
					{
						LinkedFakeProjectile->DrawProjectileStep(DevSettings->ClientFakeProjectileColor, true);

						/* Draw an arrow from the fake projectile to the authoritative projectile at each timestamp.
						 * Randomize this color so we can distinguish each step. */
						FColor ArrowColor = FColor::MakeRandomColor();
						DrawDebugDirectionalArrow(GetWorld(), LinkedFakeProjectile->GetActorLocation(), GetActorLocation(), 30.f, ArrowColor, false, 10.0f, 0, 0.5f);
						DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(),LinkedFakeProjectile->GetActorLocation(), 30.f, ArrowColor, false, 10.0f, 0, 0.5f);
					}
				}
			}
			/* Draw the fake projectile if desired. We don't need to check bWaitForLinkage here (see BeginPlay), but
			 * it's more efficient to do so. */
			else if (!DevSettings->bWaitForLinkage)
			{
				if (HasAuthority() && GetInstigator() && GetInstigator()->IsLocallyControlled() &&
					(DevSettings->ProjectileDebugMode == EProjectileDebugMode::PredictedVersusClient ||DevSettings->ProjectileDebugMode == EProjectileDebugMode::All))
				{
					DrawProjectileStep(DevSettings->ClientFakeProjectileColor, true);
				}
			}
		}
	}
}

void AASProjectile::DrawDetonationInfo(const FVector& Location, const FVector& Normal) const
{
	if (const UGASDeveloperSettings* DevSettings = GetDefault<UGASDeveloperSettings>())
	{
		if (DevSettings->bDrawFinalPosition)
		{
			const FColor Color = GetDebugColor();

			// Show the transform of the hit.
			DrawDebugSphere(GetWorld(), Location, 1.0f, 8, Color, false, DevSettings->DrawTime, 0, 1.0f);
			DrawDebugDirectionalArrow(GetWorld(), Location, (Location + (Normal * 25.f)), 25.f, Color, false, DevSettings->DrawTime, 0, 1.0f);

			// Show the AOE radius. AOE radius is only relevant on the server, where AOE effects are applied.
			if (AreaRadius > 0.0f && IsServerProjectile())
			{
				DrawDebugSphere(GetWorld(), GetAreaOfEffectOrigin(), AreaRadius, (AreaRadius / 16.f), Color, false, DevSettings->DrawTime);

				// If the sphere's origin isn't the projectile's location, draw it too.
				if (AreaOffset.Size() > 0.0f)
				{
					DrawDebugSphere(GetWorld(), GetAreaOfEffectOrigin(), 2.0f, 8, Color, false, DevSettings->DrawTime, 0, 2.0f);
				}
			}
		}
	}
}

void AASProjectile::DrawProjectileStep(const FColor& Color, bool bDrawDot, bool bThick) const
{
	DrawDebugSphere(GetWorld(), GetActorLocation(), CollisionComp->GetScaledSphereRadius(), 8, Color, false,  GetDefault<UGASDeveloperSettings>()->DrawTime, 0, bThick ? 1.0f : 0.0f);

	if (bDrawDot)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), 1.0f, 8, Color, false, GetDefault<UGASDeveloperSettings>()->DrawTime, 0 , 1.0f);
	}
}

FColor AASProjectile::GetDebugColor() const
{
	const UGASDeveloperSettings* DevSettings = GetDefault<UGASDeveloperSettings>();
	return (bIsFakeProjectile ?  DevSettings->ClientFakeProjectileColor :
		IsServerProjectile() ? DevSettings->ServerProjectileColor :
			DevSettings->ClientAuthoritativeProjectileColor);
}
#endif

