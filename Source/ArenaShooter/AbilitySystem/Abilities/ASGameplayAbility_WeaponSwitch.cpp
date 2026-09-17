// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameplayAbility_WeaponSwitch.h"

#include "Inventory/ASInventoryComponent.h"

UASGameplayAbility_WeaponSwitch::UASGameplayAbility_WeaponSwitch()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UASGameplayAbility_WeaponSwitch::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UASInventoryComponent* Inventory = UASInventoryComponent::FindInventoryComponent(ActorInfo->AbilitySystemComponent.Get());
	const FGameplayTag Requested = TriggerEventData ? TriggerEventData->EventTag : FGameplayTag();
	
	if (Inventory && Requested.IsValid())
	{
		if (Requested == NextTag)
		{
			Inventory->CycleSlot(1);
		}
		else if (Requested == PreviousTag)
		{
			Inventory->CycleSlot(-1);
		}
		else
		{
			const int32 Slot = SlotTags.IndexOfByKey(Requested);
			if (Slot != INDEX_NONE)
			{
				Inventory->RequestSwitch(Slot);
			}
		}
	}
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
