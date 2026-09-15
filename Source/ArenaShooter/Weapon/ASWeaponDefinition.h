// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ASWeaponDefinition.generated.h"


class UAbilitySet;
class AASWeaponCosmetic;
class UASWeaponInstance;
class USoundBase;

UCLASS(BlueprintType, Const)
class ARENASHOOTER_API UASWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	TSubclassOf<UASWeaponInstance> InstanceClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	TSubclassOf<AASWeaponCosmetic> CosmeticClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UAbilitySet> AbilitySet;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float BaseDamage = 10.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting")
	int32 BulletsPerShot = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting")
	float SpreadHalfAngleDeg = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting")
	float FireInterval = 0.1f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity", meta = (Categories = "Weapon"))
	FGameplayTag WeaponTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hotbar")
	FSlateBrush Icon;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (Categories = "Ammo"))
	FGameplayTag AmmoType;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = 0))
	int32 StartingAmmo = 100;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = 1))
	int32 FireCost = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> FPLinkedLayer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> TPLinkedLayer;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimMontage> Equip1PMontage;
	
	UPROPERTY(BlueprintReadonly, EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimMontage> Equip3PMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> EquipSound;
	
#if WITH_EDITOR
	EDataValidationResult IsDataValid(FDataValidationContext& Context) const;
#endif
};