// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ASAbilitySystemBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASAbilitySystemBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="ArenaShooter|GameplayCue")
	static FGameplayAbilityTargetDataHandle GetTargetDataFromCueParameters(const FGameplayCueParameters& Parameters);

	UFUNCTION(BlueprintCallable, Category="ArenaShooter|GameplayCue")
	static void EffectContextAddTargetData(FGameplayEffectContextHandle EffectContext, const FGameplayAbilityTargetDataHandle& TargetData);
};
