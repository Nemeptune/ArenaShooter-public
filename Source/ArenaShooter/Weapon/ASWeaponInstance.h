// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySet.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "ASWeaponInstance.generated.h"

class UASInventoryComponent;
class AASWeaponCosmetic;
class UASWeaponDefinition;
class UASAbilitySystemComponent;
struct FSlateBrush;

DECLARE_MULTICAST_DELEGATE_OneParam(FASOnWeaponAmmoChanged, UASWeaponInstance*);

UCLASS(BlueprintType, Blueprintable)
class ARENASHOOTER_API UASWeaponInstance : public UObject
{
	GENERATED_BODY()
public:
	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** grants the ability set with this as SourceObject. */
	void Initialize(UASWeaponDefinition* InDefinition);
	
	void Uninitialize();
	
	void NotifyEquipped(AASWeaponCosmetic* InCosmetic);
	void NotifyUnequipped();

	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	bool IsActive() const;
	
	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Weapon")
	void MarkFired();

	// --- Data, straight off the definition ---
	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	const UASWeaponDefinition* GetDefinition() const;

	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	FGameplayTag GetWeaponTag() const;
	
	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	FGameplayTag GetAmmoType() const;

	const FSlateBrush& GetIcon() const;
	int32 GetBulletsPerShot() const;
	float GetSpreadHalfAngleDeg() const;
	
	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	int32 GetAmmo() const;
	
	int32 GetFireCost() const;
	
	float GetBaseDamage() const;
	
	void AddAmmo(int32 Delta);
	
	// Authority
	void ConsumeAmmo(int32 Amount);
	
	// Owning Client
	void PredictSpendAmmo(int32 Amount);
	
	FASOnWeaponAmmoChanged OnAmmoChanged;

	// --- Cosmetics, for the gameplay cues ---
	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	USkeletalMeshComponent* GetWeaponMesh1P() const;

	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	USkeletalMeshComponent* GetWeaponMesh3P() const;

	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	FTransform GetMuzzleTransform(FName Socket = NAME_None) const;

protected:
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;

	UASAbilitySystemComponent* GetASC() const;
	
	AActor* GetOwningActor() const;
	
	UASInventoryComponent* GetOwningInventory() const;
	
	bool HasAuthority() const;
	bool IsLocallyControlled() const;
	
	virtual void OnEquipped() {}
	virtual void OnUnequipped() {}
	virtual void OnFired(){}
	
	UPROPERTY(Transient)
	TWeakObjectPtr<AASWeaponCosmetic> LocalCosmetic;
	
	void NotifyAmmoChanged();
	
	UFUNCTION()
	void OnRep_Ammo();

	UPROPERTY(ReplicatedUsing = OnRep_Ammo)
	int32 Ammo = 0;
	
	// owning-client display prediction
	int32  PendingShots = 0;
	int32  LastRepAmmo = 0;
	double LastLocalFire = -BIG_NUMBER;
	
	UPROPERTY(Replicated)
	TObjectPtr<UASWeaponDefinition> Definition;
	
	UPROPERTY()
	FAbilitySet_GrantedHandles GrantedHandles;
	
	/** Never replicated — every machine stamps its own clock at its own "shot happened" moment. */
	double TimeLastFired = -BIG_NUMBER;
};
