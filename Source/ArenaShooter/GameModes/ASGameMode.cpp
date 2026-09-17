// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameMode.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Inventory/ASInventoryComponent.h"

void AASGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);
	
	GiveLoadout(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerPawn));
}

void AASGameMode::GiveLoadout(UAbilitySystemComponent* AbilitySystemComponent)
{
	UASInventoryComponent* Inventory = UASInventoryComponent::FindInventoryComponent(AbilitySystemComponent);
	if (!Inventory)
	{
		return;
	}
	
	Inventory->RemoveAll();
	
	for (UASWeaponDefinition* Definition : DefaultLoadout)
	{
		Inventory->AddWeapon(Definition);
	}
	
	Inventory->SetActiveSlotAuth(InitialSlot);
}

void AASGameMode::StripLoadout(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (UASInventoryComponent* Inventory = UASInventoryComponent::FindInventoryComponent(AbilitySystemComponent))
	{
		Inventory->RemoveAll();
	}
}
