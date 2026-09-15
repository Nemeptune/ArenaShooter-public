// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ASAbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "ASAbilitySystemComponent.h"
#include "ASGameplayEffectContext.h"

FGameplayEffectContext* UASAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FASGameplayEffectContext();
}

UASAbilitySystemComponent* UASAbilitySystemGlobals::GetAbilitySystemComponentFromActor(const AActor* Actor, bool LookForComponent)
{
	if (Actor == nullptr)
	{
		return nullptr;
	}

	UAbilitySystemComponent* FoundASC = nullptr;
	
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor);
	if (ASI)
	{
		FoundASC = ASI->GetAbilitySystemComponent();
	}
	else if (LookForComponent)
	{
		FoundASC = Actor->FindComponentByClass<UAbilitySystemComponent>();
	}
 
	return Cast<UASAbilitySystemComponent>(FoundASC);
}
