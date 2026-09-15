// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModels/ASWeaponSlotViewModel.h"

void UASWeaponSlotViewModel::SetAmmo(int32 Value)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(Ammo, Value))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetAmmoText);
	}
}

void UASWeaponSlotViewModel::SetIsActive(bool Value)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bIsActive, Value))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetActiveHighlightVisibility);
	}
}

void UASWeaponSlotViewModel::SetIcon(const FSlateBrush& Value)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(Icon, Value))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetIcon);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSlotVisibility);
	}
}

FSlateBrush UASWeaponSlotViewModel::GetIcon() const
{
	return Icon;
}

FText UASWeaponSlotViewModel::GetAmmoText() const
{
	return FText::AsNumber(Ammo);
}

ESlateVisibility UASWeaponSlotViewModel::GetActiveHighlightVisibility() const
{
	return bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
}

ESlateVisibility UASWeaponSlotViewModel::GetSlotVisibility() const
{
	return  Icon.GetResourceObject() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed; 
}
