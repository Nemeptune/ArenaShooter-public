// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASGameplayAbility_FromEquipment.h"
#include "ASGameplayAbility_WeaponBase.generated.h"

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
	
private:
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
};
