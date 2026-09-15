// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Binders/ASVitalsBinder.h"

#include "AbilitySystem/ASCombatAttributeSet.h"
#include "UI/ViewModels/ASVitalsViewModel.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"

void UASVitalsBinder::Init(UASAbilitySystemComponent* ASC, UASVitalsViewModel* InViewModel)
{
	if (!ASC || !InViewModel)
	{
		return;
	}
	ViewModel = InViewModel;
	BoundASC = ASC;
	
	Activate();
}

void UASVitalsBinder::SubscribeAll()
{
	UASAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		return;
	}
	
	ASC->GetGameplayAttributeValueChangeDelegate(UASCombatAttributeSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UASCombatAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &ThisClass::HandleMaxHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UASCombatAttributeSet::GetShieldAttribute()).AddUObject(this, &ThisClass::HandleShieldChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UASCombatAttributeSet::GetMaxShieldAttribute()).AddUObject(this, &ThisClass::HandleMaxShieldChanged);
}

void UASVitalsBinder::UnSubscribeAll()
{
	if (UASAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UASCombatAttributeSet::GetHealthAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(UASCombatAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(UASCombatAttributeSet::GetShieldAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(UASCombatAttributeSet::GetMaxShieldAttribute()).RemoveAll(this);
	}
	
	BoundASC = nullptr;
	ViewModel = nullptr;
}

void UASVitalsBinder::RefreshAll()
{
	if (!ViewModel)
	{
		return;
	}
	
	UASAbilitySystemComponent* ASC = BoundASC.Get();
	// initial push
	ViewModel->SetMaxHealth(ASC ? ASC->GetNumericAttribute(UASCombatAttributeSet::GetMaxHealthAttribute()) : 0.f);
	ViewModel->SetHealth(ASC ? ASC->GetNumericAttribute(UASCombatAttributeSet::GetHealthAttribute()) : 0.f);
	ViewModel->SetMaxShield(ASC ? ASC->GetNumericAttribute(UASCombatAttributeSet::GetMaxShieldAttribute()) : 0.f);
	ViewModel->SetShield(ASC ? ASC->GetNumericAttribute(UASCombatAttributeSet::GetShieldAttribute()) : 0.f);
}

void UASVitalsBinder::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (ViewModel)
	{
		ViewModel->SetHealth(Data.NewValue);
	}
}

void UASVitalsBinder::HandleMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	if (ViewModel)
	{
		ViewModel->SetMaxHealth(Data.NewValue);
	}
}

void UASVitalsBinder::HandleShieldChanged(const FOnAttributeChangeData& Data)
{
	if (ViewModel)
	{
		ViewModel->SetShield(Data.NewValue);
	}
}

void UASVitalsBinder::HandleMaxShieldChanged(const FOnAttributeChangeData& Data)
{
	if (ViewModel)
	{
		ViewModel->SetMaxShield(Data.NewValue);
	}
}
