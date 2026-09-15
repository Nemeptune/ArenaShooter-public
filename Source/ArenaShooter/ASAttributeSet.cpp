// Fill out your copyright notice in the Description page of Project Settings.


#include "ASAttributeSet.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"

UWorld* UASAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UASAbilitySystemComponent* UASAttributeSet::GetAbilitySystemComponent() const
{
	return Cast<UASAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
