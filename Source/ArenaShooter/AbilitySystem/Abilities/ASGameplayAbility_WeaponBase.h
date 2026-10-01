// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASGameplayAbility_FromEquipment.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "ASGameplayAbility_WeaponBase.generated.h"

/**
 * One hitscan bullet as the client fired it. The server keeps only the ray and the moment: it traces the
 * ray itself, against hitboxes rewound to ViewServerTime, and never applies the hit the client found.
 */
USTRUCT()
struct FASGameplayAbilityTargetData_ShotHit : public FGameplayAbilityTargetData_SingleTargetHit
{
	GENERATED_BODY()

	/** Server time of the world the shooter's screen showed when they fired. Zero if unknown. */
	UPROPERTY()
	double ViewServerTime = 0.;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FASGameplayAbilityTargetData_ShotHit::StaticStruct();
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		FGameplayAbilityTargetData_SingleTargetHit::NetSerialize(Ar, Map, bOutSuccess);
		Ar << ViewServerTime;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FASGameplayAbilityTargetData_ShotHit> : public TStructOpsTypeTraitsBase2<FASGameplayAbilityTargetData_ShotHit>
{
	enum
	{
		WithNetSerializer = true
	};
};

/**
 * Base class for all weapon hitscan fire abilities.
 */
UCLASS(Abstract)
class ARENASHOOTER_API UASGameplayAbility_WeaponBase : public UASGameplayAbility_FromEquipment
{
	GENERATED_BODY()

public:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Weapon")
	void StartRangedWeaponTargeting();

	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Weapon")
	virtual void HandleTargetDataOnAuthority(const FGameplayAbilityTargetDataHandle& TargetData);

protected:
	virtual void PerformLocalTargeting(TArray<FHitResult>& OutHits);

	UFUNCTION(BlueprintImplementableEvent)
	void OnRangedWeaponTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetData);

	void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	/**
	 * Server, for a remote client's shot: replaces the hits the client claims with the server's own trace of the
	 * same rays, against hitboxes rewound to what the client saw. False, leaving nothing to apply, when the shot
	 * breaks the weapon's refire time, bullet count or spread.
	 */
	bool RetraceClientShot(FGameplayAbilityTargetDataHandle& InOutTargetData) const;

private:
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
};
