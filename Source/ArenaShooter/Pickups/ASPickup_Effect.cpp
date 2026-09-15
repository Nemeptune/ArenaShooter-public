// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/ASPickup_Effect.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"

bool AASPickup_Effect::GiveTo(AActor* Actor, UAbilitySystemComponent* ASC)
{
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectsToApply)
	{
		if (EffectClass)
		{
			ASC->ApplyGameplayEffectToSelf(EffectClass->GetDefaultObject<UGameplayEffect>(), 1.f, Context);
		}
	}
	
	return true;
}
