#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class APawn;
class UAbilitySystemComponent;
class UASCombatAttributeSet;
class UASInventoryComponent;
class UASWeaponInstance;

/** Reaches into a spawned player the way the game does: through its pawn's ability system. */
namespace ASTestPlayer
{
	UAbilitySystemComponent* GetAbilitySystem(const APawn* Pawn);
	const UASCombatAttributeSet* GetAttributes(const APawn* Pawn);
	UASInventoryComponent* GetInventory(const APawn* Pawn);
	UASWeaponInstance* GetWeapon(const APawn* Pawn, FGameplayTag WeaponTag);

	/** Presses and releases fire, as the player controller does for the input bound to Input.PrimaryAbility. */
	void PullTrigger(const APawn* Pawn);
}