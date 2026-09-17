// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASGameplayAbility.h"
#include "ASGameplayAbility_WeaponSwitch.generated.h"

/**
 *  Runs only on the owning client; the inventory predicts the swap and sends it to the server itself.
 */
UCLASS()
class ARENASHOOTER_API UASGameplayAbility_WeaponSwitch : public UASGameplayAbility
{
	GENERATED_BODY()
	
public:
	UASGameplayAbility_WeaponSwitch();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Weapon", meta = (Categories = "input"))
	TArray<FGameplayTag> SlotTags;
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Weapon", meta = (Categories = "input"))
	FGameplayTag NextTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Weapon", meta = (Categories = "input"))
	FGameplayTag PreviousTag;
};
