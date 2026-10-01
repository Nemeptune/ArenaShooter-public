#include "Helpers/ASTestPlayer.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ASCombatAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "Inventory/ASInventoryComponent.h"
#include "Weapon/ASWeaponInstance.h"

UAbilitySystemComponent* ASTestPlayer::GetAbilitySystem(const APawn* Pawn)
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
}

const UASCombatAttributeSet* ASTestPlayer::GetAttributes(const APawn* Pawn)
{
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystem(Pawn);
	return AbilitySystem ? AbilitySystem->GetSet<UASCombatAttributeSet>() : nullptr;
}

UASInventoryComponent* ASTestPlayer::GetInventory(const APawn* Pawn)
{
	return UASInventoryComponent::FindInventoryComponent(GetAbilitySystem(Pawn));
}

UASWeaponInstance* ASTestPlayer::GetWeapon(const APawn* Pawn, FGameplayTag WeaponTag)
{
	const UASInventoryComponent* Inventory = GetInventory(Pawn);
	return Inventory ? Inventory->GetInstanceAtSlot(Inventory->FindSlotByTag(WeaponTag)) : nullptr;
}

void ASTestPlayer::PullTrigger(const APawn* Pawn)
{
	if (UASAbilitySystemComponent* AbilitySystem = Cast<UASAbilitySystemComponent>(GetAbilitySystem(Pawn)))
	{
		const FGameplayTag FireInput = FGameplayTag::RequestGameplayTag(TEXT("Input.PrimaryAbility"));
		AbilitySystem->AbilityInputPressed(FireInput);
		AbilitySystem->AbilityInputReleased(FireInput);
	}
}