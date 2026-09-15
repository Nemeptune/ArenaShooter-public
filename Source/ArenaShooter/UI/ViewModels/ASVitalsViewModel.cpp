// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModels/ASVitalsViewModel.h"

void UASVitalsViewModel::SetHealth(float Value)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(Health, Value))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthText);
	}
}

void UASVitalsViewModel::SetMaxHealth(float Value)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, Value))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthText);
	}
}

void UASVitalsViewModel::SetShield(float Value)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(Shield, Value))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetShieldPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetShieldText);
	}
}

void UASVitalsViewModel::SetMaxShield(float Value)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MaxShield, Value))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetShieldPercent);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetShieldText);
	}
}

float UASVitalsViewModel::GetHealthPercent() const
{
	return MaxHealth > 0.f ? Health / MaxHealth : 0.f;
}

float UASVitalsViewModel::GetShieldPercent() const
{
	return MaxShield > 0.f ? Shield / MaxShield : 0.f;
}

FText UASVitalsViewModel::GetHealthText() const
{
	return FText::FromString(FString::Printf(TEXT("%d/%d"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)));
}

FText UASVitalsViewModel::GetShieldText() const
{
	return FText::FromString(FString::Printf(TEXT("%d/%d"), FMath::RoundToInt(Shield), FMath::RoundToInt(MaxShield)));
}
