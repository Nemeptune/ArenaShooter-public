// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "ASInventoryComponent.generated.h"


class UASWeaponInstance;
class UASWeaponDefinition;
struct FASSlotChangedMessage;

DECLARE_MULTICAST_DELEGATE_OneParam(FASOnActiveWeaponChanged, UASWeaponInstance*);

USTRUCT()
struct FASActiveSlotState
{
	GENERATED_BODY()

	UPROPERTY() 
	int32 SlotIndex = -1;   // index into Inventory
	UPROPERTY() 
	uint8 Seq = 0;          // which client request produced this
};

UCLASS( ClassGroup=(ArenaShooter), meta=(BlueprintSpawnableComponent) )
class ARENASHOOTER_API UASInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UASInventoryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	static UASInventoryComponent* FindInventoryComponent(const APlayerState* PlayerState);
	
	virtual void InitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ReadyForReplication() override;
	virtual void UninitializeComponent() override;
	
	// --- Authority ---
	void SpawnDefaultInventory();
	bool AddWeapon(UASWeaponDefinition* Definition);
	bool RemoveSlot(int32 Slot);
	void RemoveAll();
	
	int32 GiveAmmo(FGameplayTag AmmoType, int32 Amount);
	
	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Inventory")
	void RequestSwitch(int32 NewSlot);
	
	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Inventory")
	void CycleSlot(int32 Direction);
	
	int32 GetActiveSlot() const;
	
	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Inventory")
	UASWeaponInstance* GetInstanceAtSlot(int32 Slot) const;
	
	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Inventory")
	UASWeaponInstance* GetActiveInstance() const;
	
	FASOnActiveWeaponChanged OnActiveWeaponChanged;
	
	int32 GetCapacity() const;
	int32 FindSlotByTag(FGameplayTag Tag) const;
	bool IsSlotFilled(int32 Slot) const;
	const UASWeaponDefinition* GetDefinitionAtSlot(int32 Slot) const;
	int32 GetAmmoAtSlot(int32 Slot) const;
	bool IsLocallyControlled() const;
	
	FASSlotChangedMessage BuildSlotMessage(int32 Slot) const;

protected:
	
	UASWeaponInstance* GetWantedActiveInstance() const;
	
	void SetActiveSlotAuth(int32 NewSlot);
	
	int32 FindSlotByInstance(const UASWeaponInstance* Instance) const;
	int32 FindSlotByDefinition(const UASWeaponDefinition* Definition) const;
	int32 FindFirstEmptySlot() const;
	
	UPROPERTY(EditDefaultsOnly, Category="Inventory", meta = (ClampMin = 1))
	int32 Capacity = 5;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<TObjectPtr<UASWeaponDefinition>> DefaultInventory;
	
	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	TArray<TObjectPtr<UASWeaponInstance>> Slots;
	
	UPROPERTY(ReplicatedUsing = OnRep_ActiveSlotState)
	FASActiveSlotState ActiveSlotState;
	
	void BindInstance(UASWeaponInstance* Instance);
	void UnbindInstance(UASWeaponInstance* Instance);
	void UnbindAllInstances();
	void HandleInstanceAmmoChanged(UASWeaponInstance* Instance); 
	
	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotState();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetActiveSlot(int32 NewSlot, uint8 Seq);
	
	void RefreshActiveInstance();

	void BroadcastSlotChanged(int32 Slot) const;
	void BroadcastActiveSlotChanged(int32 OldSlot, int32 NewSlot);
	
	void AddInstanceToReplication(UASWeaponInstance* Instance);
	void RemoveInstanceFromReplication(UASWeaponInstance* Instance);

	UPROPERTY(Transient)
	TObjectPtr<UASWeaponInstance> ActiveInstance;

	// owner-local prediction state
	int32 PredictedSlot = INDEX_NONE;
	uint8 LocalSeq = 0;

	int32 LastBroadcastActiveSlot = INDEX_NONE;
	
	bool bTearingDown = false;
	
	bool CanBroadcast() const;
	void TeardownSlot(int32 Slot);
	
	TArray<TWeakObjectPtr<UASWeaponInstance>> BoundInstances;
};
