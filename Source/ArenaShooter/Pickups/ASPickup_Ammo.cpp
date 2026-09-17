// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/ASPickup_Ammo.h"

#include "AbilitySystemComponent.h"
#include "Weapon/ASAmmoReceiver.h"

bool AASPickup_Ammo::GiveTo(UAbilitySystemComponent* ASC)
{
	IASAmmoReceiver* Receiver = ASC->GetOwner()->FindComponentByInterface<IASAmmoReceiver>();

	// Carries nothing this pickup feeds — leave it standing rather than burning it.
	return Receiver && Receiver->GiveAmmo(AmmoType, AmmoAmount) > 0;
}
