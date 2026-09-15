// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/ASPickup_Ammo.h"

#include "ASAmmoReceiver.h"

bool AASPickup_Ammo::GiveTo(AActor* Actor, UAbilitySystemComponent* ASC)
{
	IASAmmoReceiver* Receiver = Cast<IASAmmoReceiver>(Actor);

	// Carries nothing this pickup feeds — leave it standing rather than burning it.
	return Receiver && Receiver->GiveAmmo(AmmoType, AmmoAmount) > 0;
}
