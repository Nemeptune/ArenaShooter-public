// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASGameplayAbility.h"
#include "ASGameplayAbility_Buff.generated.h"

class UGameplayEffect;

UENUM()
enum class EASBuffPlayloadTarget
{
	Self,
	Other
};

/**
 * Base for buffs that react to a damage event. A buff GameplayEffect grants one of these for
 * its lifetime; a gameplay event triggers it; it applies a single instant payload effect scaled
 * by how much damage actually landed.
 */
UCLASS(Abstract)
class ARENASHOOTER_API UASGameplayAbility_Buff : public UASGameplayAbility
{
	GENERATED_BODY()
	
public:
	UASGameplayAbility_Buff();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category="Buff")
	TSubclassOf<UGameplayEffect> PayloadEffect;
	
	UPROPERTY(EditDefaultsOnly, Category="Buff", meta = (Categories = "Data"))
	FGameplayTag MagnitudeTag;
	
	UPROPERTY(EditDefaultsOnly, Category="Buff", meta = (ClampMin = "0.0"))
	float Fraction = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, Category="Buff")
	EASBuffPlayloadTarget PayloadTarget = EASBuffPlayloadTarget::Self;
	
	UPROPERTY(EditDefaultsOnly, Category="Buff")
	FGameplayTagContainer PayloadTags;
};
