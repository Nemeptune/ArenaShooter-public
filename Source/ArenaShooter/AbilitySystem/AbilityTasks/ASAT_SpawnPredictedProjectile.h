// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ASAT_SpawnPredictedProjectile.generated.h"

class AASPlayerController;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpawnPredictedProjectileDelegate, AASProjectile*, SpawnedProjectile);

class AASProjectile;

USTRUCT()
struct FGameplayAbilityTargetData_ProjectileSpawnInfo : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	FVector SpawnLocation;

	UPROPERTY()
	FRotator SpawnRotation;

	UPROPERTY()
	uint32 ProjectileId;

	FGameplayAbilityTargetData_ProjectileSpawnInfo() :
		SpawnLocation(ForceInit),
		SpawnRotation(ForceInit),
		ProjectileId(0)
	{}

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FGameplayAbilityTargetData_ProjectileSpawnInfo::StaticStruct();
	}

	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("FGameplayAbilityTargetData_ProjectileSpawnInfo: (%i)"), ProjectileId);
	}

	static FGameplayAbilityTargetDataHandle MakeProjectileSpawnInfoTargetData(const FVector& SpawnLocation, const FRotator& SpawnRotation, const uint32 ProjectileId)
	{
		FGameplayAbilityTargetData_ProjectileSpawnInfo* TargetData = new FGameplayAbilityTargetData_ProjectileSpawnInfo();
		TargetData->SpawnLocation = SpawnLocation;
		TargetData->SpawnRotation = SpawnRotation;
		TargetData->ProjectileId = ProjectileId;
		FGameplayAbilityTargetDataHandle Handle;
		Handle.Data.Add(TSharedPtr<FGameplayAbilityTargetData_ProjectileSpawnInfo>(TargetData));
		return Handle;
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << SpawnLocation;
		Ar << SpawnRotation;
		Ar << ProjectileId;

		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_ProjectileSpawnInfo> : public TStructOpsTypeTraitsBase2<FGameplayAbilityTargetData_ProjectileSpawnInfo>
{
	enum
	{
		WithNetSerializer = true
	};
};

USTRUCT()
struct FDelayedProjectileInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<AASProjectile> ProjectileClass;

	UPROPERTY()
	FVector SpawnLocation;

	UPROPERTY()
	FRotator SpawnRotation;

	UPROPERTY()
	TWeakObjectPtr<AASPlayerController> ArenaPC;

	UPROPERTY()
	uint32 ProjectileId;

	FDelayedProjectileInfo() :
		ProjectileClass(nullptr),
		SpawnLocation(ForceInit),
		SpawnRotation(ForceInit),
		ArenaPC(nullptr),
		ProjectileId(0)
	{}
};

/**
 * Spawns Partial Fast-Forwarding projectile with Synchronization and Resimulation
 */
UCLASS()
class ARENASHOOTER_API UASAT_SpawnPredictedProjectile : public UAbilityTask
{
	GENERATED_BODY()
	
	UFUNCTION(BlueprintCallable, Meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "True"), Category = "Ability|Tasks")
	static UASAT_SpawnPredictedProjectile* SpawnPredictedProjectile(UGameplayAbility* OwningAbility, TSubclassOf<AASProjectile> ProjectileClass, FVector SpawnLocation, FRotator SpawnRotation);

	/** Spawn a fake projectile on the client and an authoritative projectile on the server. */
	virtual void Activate() override;

protected:
	UPROPERTY()
	TSubclassOf<AASProjectile> ProjectileClass;

	TWeakObjectPtr<AASProjectile> SpawnedFakeProj;

	FVector SpawnLocation;
	FRotator SpawnRotation;

	FTimerHandle SpawnDelayedFakeProjHandle;
	
	/** Cached spawn info for spawning a fake projectile after a delay. */
	UPROPERTY()
	FDelayedProjectileInfo DelayedProjectileInfo;

	void SpawnDelayedFakeProjectile();

   /**
   * Called locally when the projectile is spawned.
   *
   * On clients, this will return the fake projectile actor. On the server, this will return the authoritative
   * projectile actor.
   */
	UPROPERTY(BlueprintAssignable)
	FSpawnPredictedProjectileDelegate Success;

	/** Called on the client and server if either failed to spawn their projectile actor, usually because the ability's
	 * prediction key was rejected. The ability should likely be cancelled (on both machines) at this point. */
	UPROPERTY(BlueprintAssignable)
	FSpawnPredictedProjectileDelegate FailedToSpawn;

	FActorSpawnParameters GenerateSpawnParams() const;
	FActorSpawnParameters GenerateSpawnParamsForFake(const uint32 ProjectileId) const;
	FActorSpawnParameters GenerateSpawnParamsForAuth(const uint32 ProjectileId) const;

	void SendSpawnDataToServer(const FVector& InLocation, const FRotator& InRotation, uint32 InProjectileId);

	// Spawns Authoritative projectile
	void OnSpawnDataReplicated(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag Activation);
	// Cancels this task on the server if the client failed to spawn their version of projectile
	void OnSpawnDataCancelled();

	/** Sends the task cancellation to the server if the client failed. */
	void CancelServerSpawn();

	/** Destroys the client's fake projectile if this task is rejected */
	UFUNCTION()
	void OnTaskRejected();
};
