// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ASAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbilityTargetDataFilter.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ASProjectile.generated.h"

class UArrowComponent;
class UCapsuleComponent;
class UNiagaraComponent;
class UGameplayEffect;
class USphereComponent;
class UNiagaraSystem;
class AASPlayerController;
class USoundBase;
class UAudioComponent;

UENUM()
enum class EImpactEffectDirection
{
	InProjectileDirection,
	InVelocityDirection,
	FromProjectilePosition
};

USTRUCT()
struct FRepProjectileMovement
{
	GENERATED_BODY()
	
	UPROPERTY()
	FVector_NetQuantize LinearVelocity;

	UPROPERTY()
	FVector_NetQuantize Location;

	UPROPERTY()
	FRotator Rotation;

	FRepProjectileMovement()
		: LinearVelocity(ForceInit)
		, Location(ForceInit)
		, Rotation(ForceInit)
	{}
	
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = true;

		bool bOutSuccessLocal = true;
		Location.NetSerialize(Ar, Map, bOutSuccessLocal);
		bOutSuccess &= bOutSuccessLocal;
		Rotation.SerializeCompressed(Ar);
		LinearVelocity.NetSerialize(Ar, Map, bOutSuccessLocal);
		bOutSuccess &= bOutSuccessLocal;
		
		return bOutSuccess;
	}

	bool operator==(const FRepProjectileMovement& Other) const
	{
		if (LinearVelocity != Other.LinearVelocity)
		{
			return false;
		}

		if (Location != Other.Location)
		{
			return false;
		}

		if (Rotation != Other.Rotation)
		{
			return false;
		}
		
		return true;
	}

	bool operator!= (const FRepProjectileMovement& Other) const
	{
		return !(*this == Other);
	}
};

USTRUCT()
struct FDetonationInfo
{
	GENERATED_BODY()

	UPROPERTY()
	bool bDetonated;

	UPROPERTY()
	bool bHasDirectImpactTarget;

	UPROPERTY()
	TObjectPtr<AActor> OtherActor;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> OtherComp;

	UPROPERTY()
	FVector HitLocation;

	UPROPERTY()
	FVector HitNormal;

	FDetonationInfo()
		: bDetonated(false)
		, bHasDirectImpactTarget(false)
		, OtherActor(nullptr)
		, OtherComp(nullptr)
		, HitLocation(ForceInit)
		, HitNormal(ForceInit)
	{}

	FDetonationInfo(
		bool bInDetonated,
		bool bInHasDirectImpactTarget,
		AActor* InOtherActor,
		UPrimitiveComponent* InOtherComp,
		const FVector& InHitLocation,
		const FVector& InHitNormal)
		: bDetonated(bInDetonated)
		, bHasDirectImpactTarget(bInHasDirectImpactTarget)
		, OtherActor(InOtherActor)
		, OtherComp(InOtherComp)
		, HitLocation(InHitLocation)
		, HitNormal(InHitNormal)
	{}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = true;
		
		bool bOutSuccessLocal = true;
		Ar << bDetonated;
		Ar << bHasDirectImpactTarget;
		Ar << OtherActor;
		Ar << OtherComp;
		HitLocation.NetSerialize(Ar, Map, bOutSuccessLocal);
		bOutSuccess &= bOutSuccessLocal;
		HitNormal.NetSerialize(Ar, Map, bOutSuccessLocal);
		bOutSuccess &= bOutSuccessLocal;

		return bOutSuccess;
	}
};

template<>
struct TStructOpsTypeTraits<FDetonationInfo> : public TStructOpsTypeTraitsBase2<FDetonationInfo>
{
	enum
	{
		WithNetSerializer = true
	};
};

UCLASS(Abstract, Config = Game, Meta = (ChildCanTick, ToolTip = "Base class for projectiles."))
class ARENASHOOTER_API AASProjectile : public AActor, public IMomentumSource
{
	GENERATED_BODY()
	
public:
	// --- Initialization ---
	
	AASProjectile(const FObjectInitializer& ObjectInitializer);

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void PreInitializeComponents() override;

	virtual void BeginPlay() override;

	void CatchupTick(float CatchupTickDelta);

	virtual void SetLifeSpan(float InLifeSpan) override;

	// force detonate projectile on client if it detonated on server
	virtual void TornOff() override;

	virtual void LifeSpanExpired() override;

	// Disables this projectile (collision, movement, ambient effects, etc.) while it's pending destruction.
	void ShutDown();

	UFUNCTION(BlueprintImplementableEvent, Displayname = "ShutDown", Meta = (ToolTip = "Disables this projectile (FX, collision, physics, etc.) while its pending destruction. Should be used instead of EndPlay (since there's a delay before projectiles are actually destroyed, for replication purposes)."))
	void K2_ShutDown();

	virtual void Destroyed() override;

	virtual void Reset() override { Destroy(); };

protected:

	UPROPERTY()
	bool bHasSpawnedFully;

	/** Delay the projectile tear-off to make sure it had time to replicate its initial state. */
	FTimerHandle TearOffTimer;

private:

	void DisableAndHide();
	
	void StartFlightLoop();
	void StopFlightLoop();

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> FlightAudioComp;

	// --- Prediction ---
public:
	
	void InitFakeProjectileId(AASPlayerController* OwningPlayer, uint32 InProjectileId);
	
	FORCEINLINE void InitProjectileId(uint32 InProjectileId) { ProjectileId = InProjectileId; }
	
protected:
	
	bool bIsFakeProjectile;

	UPROPERTY(Replicated)
	uint32 ProjectileId;

	UPROPERTY()
	TObjectPtr<AASProjectile> LinkedFakeProjectile;

	UPROPERTY()
	TObjectPtr<AASProjectile> LinkedAuthProjectile;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Projectile")
	bool bCorrectFakeProjectilePositionOverTime;

	void LinkFakeProjectile(AASProjectile* InFakeProjectile);

	void SwitchToRealProjectile();

	virtual void Tick(float DeltaTime) override;

	void CorrectionLerpTick(float DeltaTime);

private:

	float InitialProjectileError;

	/** After the fake projectile detonates, if the real projectile doesn't detonate in time, we switch to it and
	* consider the fake projectile's detonation a missed prediction. */
	FTimerHandle SwitchToAuthTimer;

	// --- Collision ---
protected:

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Projectile")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Projectile")
	TObjectPtr<UCapsuleComponent> HitboxComp;

	// --- Movement ---

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArenaShooter|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|ProjectileBounces", Meta = (EditCondition = "ProjectileMovement->bShouldBounce"))
	bool bLimitBounces;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|ProjectileBounces", Meta = (EditCondition = "bLimitBounces"))
	int32 MaximumBounces;

	/**
	* If enabled, this projectile's velocity will be updated to face its owning player's camera vector on its first
	* bounce.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|ProjectileBounces", Meta = (EditCondition = "ProjectileMovement->bShouldBounce"))
	bool bStraightenProjectileOnBounce;

	bool bDetonated;

	bool bInOverlap;

	/** Called when this projectile bounces off a surface. Triggers the Bounce FX. */
	UFUNCTION()
	virtual void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	UFUNCTION()
	virtual void OnHitBoxOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Called when this projectile stops moving (usually because the collision component hit a blocking surface).
	* Triggers detonation. */
	UFUNCTION()
	virtual void OnStop(const FHitResult& Hit);

	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	// --- Movement replication ---

public:

	/** Gather replicated movement if bReplicateProjectileMovement is set. */
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	virtual void GatherCurrentMovement() override;

	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

	virtual FVector GetVelocity() const override;

	/** Remove the IsReplicatingMovement() condition. We use bReplicateProjectileMovement instead. */
	virtual void OnRep_ReplicatedMovement() override;

	/** Remove the IsReplicatingMovement() condition from AttachmentReplication. */
	virtual void GetReplicatedCustomConditionState(FCustomPropertyConditionState& OutActiveState) const override;

protected:

	/** Enables movement replication. Use this instead of bReplicateMovement. This should usually not be enabled by
	 * default, because we want projectiles simulate locally. Its primary use is replicating the final position of the
	 * authoritative projectile to clients, to ensure they detonate in the correct location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArenaShooter|Replication")
	bool bReplicateProjectileMovement;

	UPROPERTY(ReplicatedUsing = OnRep_RepProjectileMovement)
	FRepProjectileMovement ReplicatedProjectileMovement;

	UFUNCTION()
	void OnRep_RepProjectileMovement();

	/** Timer to defer this projectile's detonation on non-owning simulated proxies. We wait until the projectile has
	* finished resimulating to trigger its detonation. */
	FTimerHandle FinishedResimulationTimer;

	bool bFinishedResim;

	// --- Gameplay Effects --

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Projectile")
	bool bUseFilter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Projectile", Meta = (EditCondition = "bUseFilter"))
	FGameplayTargetDataFilter Filter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Projectile")
	TSubclassOf<UGameplayEffect> ImpactGameplayEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Projectile")
	EImpactEffectDirection ImpactEffectDirection;

	UFUNCTION()
	void Detonate(bool bHasDirectImpactTarget, AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal);

	UFUNCTION(BlueprintImplementableEvent, Meta = (ToolTip = "Called when the projectile detonates. \n\nThis is only called when this projectile's FX are triggered, so it will always fire exactly once per projectile on each client, and can be safely used to execute additional FX or gameplay. If \"Predict FX\" is enabled, this will be fired predictively, and may be fired twice due to missed predictions (the second time being on the authoritative projectile)."))
	void OnDetonate(bool bHasDirectImpactTarget, AActor* HitActor, UPrimitiveComponent* HitComp, FVector HitLocation, FVector HitNormal);

	/** If this is the owning client's version of the authoritative projectile, whether it should trigger FX (i.e.
	 * visuals, audio, etc.). This function is primarily checking for missed predictions. */
	bool ShouldAuthProjDetonateToOwner(bool bLog = false) const;

	UFUNCTION(BlueprintNativeEvent)
	FGameplayEffectSpecHandle MakeEffectSpec(bool bDirectImpact, const AActor* Target, const FHitResult& Hit) const;

	// --- Area of effect ---

public:

	virtual const FMomentumParams& GetMomentumParams() const override;
	
protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Area of Effect", DisplayName = "Area of Effect Radius", Meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AreaRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Area of Effect", DisplayName = "Area of Effect Offset", Meta = (EditCondition = "AreaRadius > 0.0"))
	FVector AreaOffset;

	/** GE to apply to actors within AoE Radius. To apply attenuation, implement the EffectSourceInterface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Area of Effect", DisplayName = "Area of Effect Gameplay Effect", Meta = (EditCondition = "AreaRadius > 0.0"))
	TSubclassOf<UGameplayEffect> AreaGameplayEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Area of Effect", DisplayName = "Skip AOE Effect for Impact Target?", Meta = (EditCondition = "AreaRadius > 0.0"))
	bool bSkipAreaEffectForImpactTarget;

	// --- Knockback ---
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Area of Effect")
	FMomentumParams MomentumParams;

	// Full knockback strength in this radius
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Area of Effect", Meta = (ClampMin = "0.0", EditCondition = "AreaRadius > 0.f"))
	float AreaInnerRadius = 100.f;

	// Targets nearer than this skip the line of sight check
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Area of Effect", Meta = (ClampMin = "0.0", EditCondition = "AreaRadius > 0.f"))
	float CollisionTraceSkipRadius = 120.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float ImpactDamage = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float AreaDamage = 30.f;

	// How far to lift the alternate LOS trace origins off the surface the blast hit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Area of Effect", Meta = (ClampMin = "0.0", EditCondition = "AreaRadius > 0.f"))
	float BlastOriginLift = 45.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Area of Effect")
	bool bAffectInstigator = true;

	FORCEINLINE FVector GetAreaOfEffectOrigin() const;

	float GetRadialFalloff(float Distance) const;

	bool HasBlastLineOfSight(const FVector& Origin, const AActor* Target, const FVector& TargetPoint) const;

private:

	void ApplyEffectToTarget(const bool bDirectImpact, const AActor* Target, const FHitResult& Hit, float DamageScale = 1.f) const;

	// --- Lifetime ---

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Projectile", Meta = (Units = "Seconds"))
	float LifeSpanAfterDetonation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Projectile")
	bool bDetonateOnLifeSpanEnd;

	// --- FX ---

protected:
	/** Whether this projectile's impact and detonation FX should be predicted. We generally don't want to predict
	 * big or slow projectiles with large AOE effects (like a rocket's explosion VFX), but we can predict the impact of
	 * smaller, fast projectiles that don't have AOE effects (like a shuriken's missed impact VFX). */
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Projectile|FX")
	bool bPredictFX;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Projectile|FX")
	FProjectileFX DetonationFX;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Projectile|FX")
	FProjectileFX BounceFX;
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Projectile|FX")
	TObjectPtr<USoundBase> FlightLoopSound;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Projectile|FX", meta = (ClampMin = 0, Units = "Seconds"))
	float FlightLoopFadeTime = 0.05f;

	/** Minimum velocity at which this projectile will trigger FX when bouncing. Note that this is relative to the
	 * normal of the bounce, to prevent sliding. */
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Projectile|FX")
	float BounceVelocityFXThreshold;

	/** Timer to visually shut down this projectile (hide its meshes, kill its VFX, etc.) before destroying it.*/
	FTimerHandle ShutDownTimer;

	/** Whether this projectile was used for FX. If a projectile isn't used for FX, it will always be hidden (or
	 * destroyed, if safe) when shut down. Otherwise, it will be kept visible according to LifeSpanAfterDetonation. */
	bool bTriggeredFX;

	// --- Internals ---

protected:

	/** The owning player's controller, cached in case our instigator dies during the projectile's lifetime. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "ArenaShooter|Projectile")
	TWeakObjectPtr<AASPlayerController> ASPlayerController;

	/** The transform with which this projectile was spawned. Used to resimulate the projectile on non-owning simulated
	 * proxies, to prevent projectiles from appearing far ahead of where they were spawned. */
	UPROPERTY(Replicated)
	FTransform SpawnTransform;

	/* Used to "straighten out" bouncing projectiles' movement after the first bounce, if desired.*/
	UPROPERTY(Replicated)
	FRotator InitialOwnerAimRotation;

	bool bHasBounced;

	int32 BounceCount;

	UPROPERTY(ReplicatedUsing = OnRep_DetonationInfo)
	FDetonationInfo DetonationInfo;

	/** 
	* Triggers detonation on non-owning simulated proxies. Since simulated proxies rewind and resimulate, they're
	* always behind the authoritative projectile, which makes their local hit detection unreliable. Instead of letting
	* them trigger their detonation locally, we wait for the server to tell us that the authoritative projectile
	* detonated.
	*
	* If detonation info is sent by the server before the simulated proxy has finished spawning, we start our
	* resimulation early (instead of waiting for BeginPlay) and adjust our resimulation speed to ensure the projectile
	* is always at least briefly visible. Otherwise, we extrapolate how much time is left in our resimulation
	* (depending on our distance from the detonation location) and set a timer to detonate when we finish.
	*/
	UFUNCTION()
	void OnRep_DetonationInfo();

	void DetonateWithDetonationInfo();

	FORCEINLINE bool IsServerProjectile() const;

private:

	/** How long to delay tearing off this projectile on the server to ensure the initial replication is sent first. */
	UPROPERTY(Config)
	float TearOffDelay;

	/** The minimum amount of time that this projectile needs to be visible. If this projectile detonates on spawn, it
	 * will be rewound and resimulated at a low speed to ensure it's at least briefly visible to players. */
	UPROPERTY(Config)
	float MinLifetime;

#if WITH_EDITOR

	// --- Debugging ---
public:

	void DrawDebug();

	void DrawDetonationInfo(const FVector& Location, const FVector& Normal) const;

	void DrawProjectileStep(const FColor& Color, bool bDrawDot = false, bool bThick = false) const;

	FColor GetDebugColor() const;

	/** Looping timer for drawing each step of a projectile's trajectory. */
	FTimerHandle DrawDebugTimer;
	
#endif
};
