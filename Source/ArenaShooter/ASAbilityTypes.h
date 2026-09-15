#pragma once

#include "CoreMinimal.h"
#include "ASAbilityTypes.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

USTRUCT()
struct FProjectileDecalInfo
{
	GENERATED_BODY()

public:
	FProjectileDecalInfo();
	
	void SpawnDecal(const UObject* WorldContextObject, const FVector& Location, const FRotator& Rotation, USceneComponent* AttachToComponent);

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInstance> DecalMaterial;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPreserveRatio = "True"))
	FVector DecalSize;

	UPROPERTY(EditDefaultsOnly, Meta = (ClampMin = "0.0", Units = "Seconds"))
	float FadeOutStartDelay;

	UPROPERTY(EditDefaultsOnly, Meta = (ClampMin = "0.0", Units = "Seconds"))
	float FadeOutDuration;
};

USTRUCT()
struct FSpawnedProjectileFX
{
	GENERATED_BODY()

	FSpawnedProjectileFX() {}

	void StopEffects();

	TArray<TObjectPtr<UNiagaraComponent>> SpawnedParticles;

	TArray<TObjectPtr<UAudioComponent>> SpawnedSounds;
};

USTRUCT()
struct FProjectileFX
{
	GENERATED_BODY()

public:
	FProjectileFX(){}

	void ExecuteEffects(const UObject* WorldContextObject, const FVector& Location, const FRotator& Rotation, USceneComponent* AttachComponent, bool bSkipDecal = false);

	void StopEffects() { SpawnedFX.StopEffects(); }
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> Particles;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditDefaultsOnly)
	FProjectileDecalInfo Decal;

private:

	FSpawnedProjectileFX SpawnedFX;
};

USTRUCT(BlueprintType)
struct FMomentumParams
{
	GENERATED_BODY()

	// Impulse at blast centre, BEFORE radial falloff and BEFORE division by the CMC's Mass
	UPROPERTY(EditDefaultsOnly, Category = "Momentum")
	float BaseMomentum = 110000.f;

	// Rocket-jump boost
	UPROPERTY(EditDefaultsOnly, Category = "Momentum")
	float SelfMomentumBoost = 1.25;

	UPROPERTY(EditDefaultsOnly, Category = "Momentum")
	bool bSelfMomentumBoostOnlyZ = true;

	// Guarantee an upward component while the victim is walking
	UPROPERTY(EditDefaultsOnly, Category = "Momentum")
	bool bForceZMomentum = true;

	UPROPERTY(EditDefaultsOnly, Category = "Momentum", Meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ForceZMomentumPct = 0.6f;
};

UINTERFACE()
class UMomentumSource : public UInterface
{
	GENERATED_BODY()
};

class IMomentumSource
{
	GENERATED_BODY()

public:

	virtual const FMomentumParams& GetMomentumParams() const = 0;
};


