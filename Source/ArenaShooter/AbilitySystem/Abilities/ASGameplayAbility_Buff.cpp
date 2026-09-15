// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameplayAbility_Buff.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UASGameplayAbility_Buff::UASGameplayAbility_Buff()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void UASGameplayAbility_Buff::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	
	if (!TriggerEventData || !MyASC || !Avatar || !PayloadEffect || Fraction <= 0.f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}
	
	const float Magnitude = TriggerEventData->EventMagnitude * Fraction;
	if (Magnitude <= 0.f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}
	
	const AActor* Recipient = Avatar;
	if (PayloadTarget == EASBuffPlayloadTarget::Other)
	{
		Recipient = (TriggerEventData->Instigator.Get() == Avatar) 
		? TriggerEventData->Target.Get() 
		: TriggerEventData->Instigator.Get();
	}
	
	UAbilitySystemComponent* RecipientASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Recipient);
	if (!RecipientASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}
	
	FGameplayEffectContextHandle Context = MyASC->MakeEffectContext();
	Context.AddInstigator(Avatar, Avatar);
	
	const FGameplayEffectSpecHandle Spec = MyASC->MakeOutgoingSpec(PayloadEffect, GetAbilityLevel(), Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MagnitudeTag, Magnitude);
		for (const FGameplayTag& Tag : PayloadTags)
		{
			Spec.Data->AddDynamicAssetTag(Tag);
		}
		
		MyASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), RecipientASC);
	}
	
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
