#pragma once

#include "CoreMinimal.h"
#include "ASInventoryMessages.generated.h"

class AActor;
class UASWeaponDefinition;

/** One slot's contents changed — filled, emptied, or its ammo moved.
 *  Published on Inventory.Message.SlotChanged. */
USTRUCT(BlueprintType)
struct FASSlotChangedMessage
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<AActor> Owner = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SlotIndex = INDEX_NONE;
	
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UASWeaponDefinition> Definition = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Ammo = 0;
};

/** The selected slot changed. Published on Inventory.Message.ActiveSlotChanged. */
USTRUCT(BlueprintType)
struct FASActiveSlotChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<AActor> Owner = nullptr;

	/** What listeners were last told — not what the model previously held. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 OldSlot = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 NewSlot = INDEX_NONE;
};