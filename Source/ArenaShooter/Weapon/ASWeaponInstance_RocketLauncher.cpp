// Fill out your copyright notice in the Description page of Project Settings.


#include "ASWeaponInstance_RocketLauncher.h"

#include "ASWeaponCosmetic_RocketLauncher.h"
#include "ASWeaponDefinition.h"

void UASWeaponInstance_RocketLauncher::OnFired()
{
	Super::OnFired();
	
	UWorld* World = GetWorld();
	if (!World || !Definition || Definition->FireInterval <=0.f)
	{
		ApplyLoadedRoundVisibility(true);
		return;
	}
	
	ApplyLoadedRoundVisibility(false);
	
	TWeakObjectPtr<UASWeaponInstance_RocketLauncher> WeakThis(this);
	World->GetTimerManager().SetTimer(ReloadVisualTimer, [WeakThis]()
	{
		if (UASWeaponInstance_RocketLauncher* Self = WeakThis.Get())
		{
			Self->ApplyLoadedRoundVisibility(true);
		}
	}, Definition->FireInterval, false);
}

void UASWeaponInstance_RocketLauncher::OnEquipped()
{
	Super::OnEquipped();
	
	ApplyLoadedRoundVisibility(CanFire());
}

void UASWeaponInstance_RocketLauncher::ApplyLoadedRoundVisibility(bool bLoaded)
{
	if (AASWeaponCosmetic_RocketLauncher* RocketCosmetic = Cast<AASWeaponCosmetic_RocketLauncher>(LocalCosmetic.Get()))
	{
		RocketCosmetic->SetLoadedRoundVisible(IsActive() && bLoaded);
	}
}
