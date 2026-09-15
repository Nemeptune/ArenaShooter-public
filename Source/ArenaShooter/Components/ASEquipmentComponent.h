// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ASEquipmentComponent.generated.h"


class UASInventoryComponent;
class IASWeaponHolder;
class UAbilitySystemComponent;
class AASWeaponCosmetic;
class UASWeaponInstance;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ARENASHOOTER_API UASEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UASEquipmentComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	void BindToInventory();

	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Equipment")
	UASWeaponInstance* GetEquippedWeapon() const { return EquippedWeapon; }

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

protected:
	void EquipWeapon(UASWeaponInstance* Instance);
	
	void SpawnCosmetic();      // local, every machine, no authority gate
	void DestroyCosmetic();
	void PlayEquipMontages();  // also sets the IsChanging lock + timer
	void PlayEquipSound();
	void ApplyEquipTags();
	void RemoveEquipTags();
	void ClearEquipLock();

	IASWeaponHolder* GetHolder() const;
	UAbilitySystemComponent* GetASC() const;
	bool IsAuthorityOrLocal() const;
	bool IsLiveAvatar() const;

	UPROPERTY(Transient) 
	TObjectPtr<UASWeaponInstance> EquippedWeapon;
	UPROPERTY(Transient) 
	TObjectPtr<AASWeaponCosmetic> Cosmetic;

	TWeakObjectPtr<UASInventoryComponent> BoundInventory;
	FTimerHandle EquipLockTimer;
};
