// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "ASAbilitySystemGlobals.generated.h"

class UASAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

	virtual FGameplayEffectContext* AllocGameplayEffectContext() const;
	
public:
	
	static UASAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor, bool LookForComponent=true);
};
