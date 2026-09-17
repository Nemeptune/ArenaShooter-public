// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Binders/ASHotbarBinder.h"

#include "Character/ASCharacter.h"
#include "Messages/ASMessageTags.h"
#include "Inventory/ASInventoryComponent.h"
#include "UI/ASHUDWidget.h"
#include "Player/ASPlayerController.h"
#include "UI/ViewModels/ASWeaponSlotViewModel.h"
#include "Inventory/ASInventoryMessages.h"
#include "Weapon/ASWeaponDefinition.h"
#include "AbilitySystemBlueprintLibrary.h"

void UASHotbarBinder::Init(AASPlayerController* PC, UASHUDWidget* InHUDWidget)
{
	if (!PC || !InHUDWidget)
	{
		return;
	}
	HUDWidget = InHUDWidget;
	BoundPC = PC;
	BoundPS = PC->PlayerState;
	
	Activate();
}

void UASHotbarBinder::SubscribeAll()
{
	if (AASPlayerController* PC = BoundPC.Get())
	{
		PC->OnPossessedPawnChanged.AddDynamic(this, &UASHotbarBinder::HandlePawnChanged);
		
		LocalCharacter = Cast<AASCharacter>(PC->GetPawn());
	}
	
	UGameplayMessageSubsystem& Bus = UGameplayMessageSubsystem::Get(this);
	SlotChangedHandle = Bus.RegisterListener(FASMessageTags::Inventory_SlotChanged, this, &UASHotbarBinder::OnSlotChanged);
	ActiveSlotChangedHandle = Bus.RegisterListener(FASMessageTags::Inventory_ActiveSlotChanged, this, &UASHotbarBinder::OnActiveSlotChanged);
}

void UASHotbarBinder::UnSubscribeAll()
{
	if (AASPlayerController* PC = BoundPC.Get())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UASHotbarBinder::HandlePawnChanged);
	}
	
	if (SlotChangedHandle.IsValid())
	{
		SlotChangedHandle.Unregister();
	}
	
	if (ActiveSlotChangedHandle.IsValid())
	{
		ActiveSlotChangedHandle.Unregister();
	}
	
	BoundPC = nullptr;
	BoundPS = nullptr;
	LocalCharacter = nullptr;
	HUDWidget = nullptr;
	ActiveSlotVM = nullptr;
	
	SlotsVM.Reset();
}

void UASHotbarBinder::EnsureSlots(int32 NumSlots)
{
	if (!HUDWidget || NumSlots <= 0 || SlotsVM.Num() == NumSlots)
	{
		return;
	}

	SlotsVM.Reset();
	ActiveSlotVM = nullptr;

	TArray<UASWeaponSlotViewModel*> Raw;
	Raw.Reserve(NumSlots);
	for (int32 i = 0; i < NumSlots; ++i)
	{
		UASWeaponSlotViewModel* VM = NewObject<UASWeaponSlotViewModel>(this);
		SlotsVM.Add(VM);
		Raw.Add(VM);
	}

	HUDWidget->InitWeaponSlots(Raw);
}

void UASHotbarBinder::OnSlotChanged(FGameplayTag, const FASSlotChangedMessage& Message)
{
	if (Message.Owner.Get() != BoundPS.Get())
	{
		return;
	}
	ApplySlot(Message);
}

void UASHotbarBinder::ApplySlot(const FASSlotChangedMessage& Message)
{
	if (!SlotsVM.IsValidIndex(Message.SlotIndex))
	{
		return;
	}
	
	UASWeaponSlotViewModel* VM = SlotsVM[Message.SlotIndex];
	
	VM->SetIcon(Message.Definition ?  Message.Definition->Icon : FSlateBrush());
	VM->SetAmmo(Message.Ammo);
}

void UASHotbarBinder::OnActiveSlotChanged(FGameplayTag Channel, const FASActiveSlotChangedMessage& Message)
{
	if (Message.Owner.Get() != BoundPS.Get())
	{
		return;
	}
	
	ApplyActiveSlot(Message.NewSlot);
}

void UASHotbarBinder::ApplyActiveSlot(int32 NewSlot)
{
	if (ActiveSlotVM)
	{
		ActiveSlotVM->SetIsActive(false);
	}
	
	ActiveSlotVM = SlotsVM.IsValidIndex(NewSlot) ? SlotsVM[NewSlot].Get() : nullptr;
	
	if (ActiveSlotVM)
	{
		ActiveSlotVM->SetIsActive(true);
	}
}

void UASHotbarBinder::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	AASCharacter* NewCharacter = Cast<AASCharacter>(NewPawn);

	if (NewCharacter == LocalCharacter) return;

	LocalCharacter = NewCharacter;
	RefreshAll();
}

void UASHotbarBinder::RefreshAll()
{
	UASInventoryComponent* Inv = UASInventoryComponent::FindInventoryComponent(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BoundPS.Get()));

	EnsureSlots(Inv ? Inv->GetCapacity() : 0);
	if (SlotsVM.Num() == 0)
	{
		return;
	}

	ApplyActiveSlot(Inv ? Inv->GetActiveSlot() : INDEX_NONE);

	for (int32 i = 0; i < SlotsVM.Num(); ++i)
	{
		FASSlotChangedMessage Empty;
		Empty.SlotIndex = i;
		ApplySlot(Inv ? Inv->BuildSlotMessage(i) : Empty);
	}
}