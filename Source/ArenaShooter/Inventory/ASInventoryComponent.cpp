// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/ASInventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "System/ASLogChannels.h"
#include "Messages/ASMessageTags.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/ASInventoryMessages.h"
#include "System/ASProfiling.h"
#include "Weapon/ASWeaponDefinition.h"
#include "Weapon/ASWeaponInstance.h"

UASInventoryComponent::UASInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
	bReplicateUsingRegisteredSubObjectList = true;
	PrimaryComponentTick.bCanEverTick = false;
}

UASInventoryComponent* UASInventoryComponent::FindInventoryComponent(const UAbilitySystemComponent* AbilitySystem)
{
	const AActor* Owner = AbilitySystem ? AbilitySystem->GetOwner() : nullptr;
	return Owner ? Owner->FindComponentByClass<UASInventoryComponent>() : nullptr;
}

void UASInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
	if (GetOwner()->HasAuthority())
	{
		Slots.SetNum(Capacity);
	}
}

void UASInventoryComponent::UninitializeComponent()
{
	bTearingDown = true;
	UnbindAllInstances();
	RemoveAll();
	Super::UninitializeComponent();
}

void UASInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UASInventoryComponent, Slots);
	DOREPLIFETIME(UASInventoryComponent, ActiveSlotState);
}

void UASInventoryComponent::ReadyForReplication()
{
	Super::ReadyForReplication();
	
	if (IsUsingRegisteredSubObjectList())
	{
		for (UASWeaponInstance* Instance : Slots)
		{
			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

void UASInventoryComponent::AddInstanceToReplication(UASWeaponInstance* Instance)
{
	if (Instance && IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		AddReplicatedSubObject(Instance);
	}
}

void UASInventoryComponent::RemoveInstanceFromReplication(UASWeaponInstance* Instance)
{
	if (Instance && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(Instance);
	}
}

bool UASInventoryComponent::CanBroadcast() const
{
	return !bTearingDown && GetWorld() != nullptr && IsLocallyControlled();
}

bool UASInventoryComponent::AddWeapon(UASWeaponDefinition* Definition)
{
	if (!GetOwner()->HasAuthority() || !Definition)
	{
		return false;
	}
	
	if (FindSlotByDefinition(Definition) != INDEX_NONE)
	{
		return false;
	}
	
	const int32 Slot = FindFirstEmptySlot();
	if (Slot == INDEX_NONE)
	{
		UE_LOG(LogAS, Warning, TEXT("AddWeapon: no free slot for %s (Entries=%d, Capacity=%d)"),
		*GetNameSafe(Definition), Slots.Num(), Capacity);
		return false;
	}
	
	TSubclassOf<UASWeaponInstance> InstanceClass = Definition->InstanceClass;
	if (InstanceClass == nullptr)
	{
		InstanceClass = UASWeaponInstance::StaticClass();
	}
	
	UASWeaponInstance* Instance = NewObject<UASWeaponInstance>(GetOwner(), InstanceClass);
	Slots[Slot] = Instance;
	BindInstance(Instance);
	
	Instance->Initialize(Definition);
	AddInstanceToReplication(Instance);

	BroadcastSlotChanged(Slot);
	return true;
}

void UASInventoryComponent::TeardownSlot(int32 Slot)
{
	UASWeaponInstance* Instance = GetInstanceAtSlot(Slot);
	if (!Instance)
	{
		return;
	}

	Slots[Slot] = nullptr;
	if (ActiveInstance == Instance)
	{
		ActiveInstance = nullptr;
	}

	RemoveInstanceFromReplication(Instance);
	UnbindInstance(Instance);
	Instance->Uninitialize();
}

bool UASInventoryComponent::RemoveSlot(int32 Slot)
{
	if (!GetOwner()->HasAuthority() || !IsSlotFilled(Slot))
	{
		return false;
	}

	if (Slot == ActiveSlotState.SlotIndex)
	{
		SetActiveSlotAuth(INDEX_NONE);
	}

	TeardownSlot(Slot);
	BroadcastSlotChanged(Slot);
	return true;
}

void UASInventoryComponent::RemoveAll()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	SetActiveSlotAuth(INDEX_NONE);

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		TeardownSlot(i);
	}

	// Publish only after everything is down, so a listener that reads other slots during
	// the message sees a consistent empty inventory rather than a half-torn-down one.
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		BroadcastSlotChanged(i);
	}
}

int32 UASInventoryComponent::GiveAmmo(FGameplayTag AmmoType, int32 Amount)
{
	if (!GetOwner()->HasAuthority() || !AmmoType.IsValid() || Amount <= 0)
	{
		return 0;
	}

	int32 NumFed = 0;
	for (const TObjectPtr<UASWeaponInstance>& Instance : Slots)
	{
		if (Instance && Instance->GetAmmoType().MatchesTag(AmmoType))
		{
			Instance->AddAmmo(Amount);
			++NumFed;
		}
	}

	return NumFed;
}

UASWeaponInstance* UASInventoryComponent::GetWantedActiveInstance() const
{
	return GetInstanceAtSlot(GetActiveSlot());
}

int32 UASInventoryComponent::GetActiveSlot() const
{
	return IsLocallyControlled() ? PredictedSlot : ActiveSlotState.SlotIndex;
}

void UASInventoryComponent::RequestSwitch(int32 NewSlot)
{
	if (!IsLocallyControlled() || !IsSlotFilled(NewSlot) || NewSlot == PredictedSlot)
	{
		return;
	}
	
	const int32 OldSlot = PredictedSlot;
	PredictedSlot = NewSlot;
	++LocalSeq;
	
	if (!GetOwner()->HasAuthority())
	{
		RefreshActiveInstance();
		BroadcastActiveSlotChanged(OldSlot, NewSlot);
	}

	ServerSetActiveSlot(NewSlot, LocalSeq);
}

bool UASInventoryComponent::ServerSetActiveSlot_Validate(int32 NewSlot, uint8 Seq)
{
	return true;
}

void UASInventoryComponent::ServerSetActiveSlot_Implementation(int32 NewSlot, uint8 Seq)
{
	ActiveSlotState.Seq = Seq;
	
	if (!IsSlotFilled(NewSlot))
	{
		return;
	}
	
	const int32 OldSlot = ActiveSlotState.SlotIndex;
	ActiveSlotState.SlotIndex = NewSlot;
	
	RefreshActiveInstance();
	BroadcastActiveSlotChanged(OldSlot, NewSlot);
}

void UASInventoryComponent::SetActiveSlotAuth(int32 NewSlot)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	if (NewSlot != INDEX_NONE && !IsSlotFilled(NewSlot))
	{
		return;
	}
	
	const int32 OldSlot = ActiveSlotState.SlotIndex;
	ActiveSlotState.SlotIndex = NewSlot;
	
	if (IsLocallyControlled())
	{
		PredictedSlot = NewSlot;
	}
	
	RefreshActiveInstance();
	BroadcastActiveSlotChanged(OldSlot, NewSlot);
}

void UASInventoryComponent::CycleSlot(int32 Direction)
{
	if (!IsLocallyControlled() || Capacity < 2 || Direction == 0)
	{
		return;
	}
	
	const int32 Start = (PredictedSlot < 0) ? (Direction > 0 ? Capacity -1 : 0) : PredictedSlot;
	
	int32 NewIndex = Start;
	do
	{
		NewIndex = (NewIndex + Direction + Capacity) % Capacity;
		if (IsSlotFilled(NewIndex))
		{
			RequestSwitch(NewIndex);
			return;
		}
	} while (NewIndex != Start);
}

void UASInventoryComponent::RefreshActiveInstance()
{
	UASWeaponInstance* Wanted = GetWantedActiveInstance();
	if (ActiveInstance == Wanted)
	{
		return;
	}

	ActiveInstance = Wanted;
	OnActiveWeaponChanged.Broadcast(ActiveInstance);
}

void UASInventoryComponent::OnRep_ActiveSlotState()
{
	if (IsLocallyControlled())
	{
		// Only adopt the server's value once it has caught up to our latest request.
		if (ActiveSlotState.Seq == LocalSeq && ActiveSlotState.SlotIndex != PredictedSlot)
		{
			CSV_CUSTOM_STAT(ArenaShooter, SlotCorrections, 1, ECsvCustomStatOp::Accumulate);
			const int32 OldSlot = PredictedSlot;
			PredictedSlot = ActiveSlotState.SlotIndex;
			RefreshActiveInstance();
			BroadcastActiveSlotChanged(OldSlot, PredictedSlot);
		}
		return;
	}
	
	RefreshActiveInstance();
	BroadcastActiveSlotChanged(LastBroadcastActiveSlot, ActiveSlotState.SlotIndex);
}

void UASInventoryComponent::BindInstance(UASWeaponInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	Instance->OnAmmoChanged.RemoveAll(this);
	Instance->OnAmmoChanged.AddUObject(this, &UASInventoryComponent::HandleInstanceAmmoChanged);
	BoundInstances.AddUnique(Instance);
}

void UASInventoryComponent::UnbindInstance(UASWeaponInstance* Instance)
{
	if (Instance)
	{
		Instance->OnAmmoChanged.RemoveAll(this);
	}
	BoundInstances.Remove(Instance);
}

void UASInventoryComponent::UnbindAllInstances()
{
	for (const TWeakObjectPtr<UASWeaponInstance>& Weak : BoundInstances)
	{
		if (UASWeaponInstance* Instance = Weak.Get())
		{
			Instance->OnAmmoChanged.RemoveAll(this);
		}
	}
	BoundInstances.Reset();
}

void UASInventoryComponent::HandleInstanceAmmoChanged(UASWeaponInstance* Instance)
{
	BroadcastSlotChanged(FindSlotByInstance(Instance));
}

void UASInventoryComponent::OnRep_Slots()
{
	UnbindAllInstances();

	for (UASWeaponInstance* Instance : Slots)
	{
		BindInstance(Instance);
	}

	RefreshActiveInstance();

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		BroadcastSlotChanged(i);
	}
}

UASWeaponInstance* UASInventoryComponent::GetInstanceAtSlot(int32 Slot) const
{
	return Slots.IsValidIndex(Slot) ? Slots[Slot].Get() : nullptr;
}

int32 UASInventoryComponent::FindSlotByInstance(const UASWeaponInstance* Instance) const
{
	return Instance ? Slots.IndexOfByKey(Instance) : INDEX_NONE;
}

int32 UASInventoryComponent::FindSlotByTag(FGameplayTag Tag) const
{
	return Slots.IndexOfByPredicate([Tag](const TObjectPtr<UASWeaponInstance>& I)
	{
		return I && I->GetWeaponTag().MatchesTagExact(Tag);
	});
}

int32 UASInventoryComponent::FindSlotByDefinition(const UASWeaponDefinition* Definition) const
{
	return Slots.IndexOfByPredicate([Definition](const TObjectPtr<UASWeaponInstance>& I)
	{
		return I && I->GetDefinition() == Definition;
	});
}

int32 UASInventoryComponent::FindFirstEmptySlot() const
{
	return Slots.IndexOfByPredicate([](const TObjectPtr<UASWeaponInstance>& I)
	{
		return I == nullptr;
	});
}

bool UASInventoryComponent::IsSlotFilled(int32 Slot) const
{
	return GetInstanceAtSlot(Slot) != nullptr;
}

const UASWeaponDefinition* UASInventoryComponent::GetDefinitionAtSlot(int32 Slot) const
{
	const UASWeaponInstance* Instance = GetInstanceAtSlot(Slot);
	return Instance ? Instance->GetDefinition() : nullptr;
}

int32 UASInventoryComponent::GetAmmoAtSlot(int32 Slot) const
{
	const UASWeaponInstance* Instance = GetInstanceAtSlot(Slot);
	return Instance ? Instance->GetAmmo() : 0;
}

bool UASInventoryComponent::IsLocallyControlled() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasLocalNetOwner();
}

struct FASSlotChangedMessage UASInventoryComponent::BuildSlotMessage(int32 Slot) const
{
	FASSlotChangedMessage Message;
	Message.Owner = GetOwner();
	Message.SlotIndex = Slot;
	
	if (const UASWeaponInstance* Instance = GetInstanceAtSlot(Slot))
	{
		Message.Definition = const_cast<UASWeaponDefinition*>(Instance->GetDefinition());
		Message.Ammo = Instance->GetAmmo();
	}
	
	return Message;
}

void UASInventoryComponent::BroadcastSlotChanged(int32 Slot) const
{
	if (!CanBroadcast() || !Slots.IsValidIndex(Slot))
	{
		return;
	}
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(FASMessageTags::Inventory_SlotChanged, BuildSlotMessage(Slot));
}

void UASInventoryComponent::BroadcastActiveSlotChanged(int32 OldSlot, int32 NewSlot)
{
	if (!CanBroadcast())
	{
		return;
	}
	
	LastBroadcastActiveSlot = NewSlot;
	
	FASActiveSlotChangedMessage Message;
	Message.Owner = GetOwner();
	Message.OldSlot = OldSlot;
	Message.NewSlot = NewSlot;
	
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(FASMessageTags::Inventory_ActiveSlotChanged, Message);
}

UASWeaponInstance* UASInventoryComponent::GetActiveInstance() const
{
	return ActiveInstance;
}

int32 UASInventoryComponent::GetCapacity() const
{
	return Capacity;
}


