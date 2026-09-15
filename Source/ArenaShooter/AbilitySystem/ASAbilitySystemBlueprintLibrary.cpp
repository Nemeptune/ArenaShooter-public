// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ASAbilitySystemBlueprintLibrary.h"
#include "ASGameplayEffectContext.h"

FGameplayAbilityTargetDataHandle UASAbilitySystemBlueprintLibrary::GetTargetDataFromCueParameters(const FGameplayCueParameters& Parameters)
{
	if (const FGameplayEffectContext* Context = Parameters.EffectContext.Get())
	{
		if (Context->GetScriptStruct()->IsChildOf(FASGameplayEffectContext::StaticStruct()))
		{
			return static_cast<const FASGameplayEffectContext*>(Context)->TargetData;
		}
	}
	
	return FGameplayAbilityTargetDataHandle();
}

void UASAbilitySystemBlueprintLibrary::EffectContextAddTargetData(FGameplayEffectContextHandle EffectContext, const FGameplayAbilityTargetDataHandle& TargetData)
{
	if (FGameplayEffectContext* Context = EffectContext.Get())
	{
		if (Context->GetScriptStruct()->IsChildOf(FASGameplayEffectContext::StaticStruct()))
		{
			static_cast<FASGameplayEffectContext*>(Context)->TargetData.Append(TargetData);
		}
	}
}
