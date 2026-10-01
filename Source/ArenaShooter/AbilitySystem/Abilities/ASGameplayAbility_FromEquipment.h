// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASGameplayAbility.h"
#include "ASGameplayAbility_FromEquipment.generated.h"

class UASWeaponInstance;
class UGameplayEffect;

/**
 * An ability granted by, and associated with, a weapon instance.
 */
UCLASS()
class ARENASHOOTER_API UASGameplayAbility_FromEquipment : public UASGameplayAbility
{
	GENERATED_BODY()
	
public:	
	UASGameplayAbility_FromEquipment();
	
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
	/** The weapon that granted this ability. it is the spec's SourceObject. */
	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	UASWeaponInstance* GetSourceWeapon() const;

	/** Stamps the instance's refire timestamp. Call after a successful CommitAbility. */
	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Weapon")
	void NotifyWeaponFired();

	/**
	 * Server: moves where a remote client says its shot started to where the shooter could have fired from.
	 * The claim stands within the shooter's movement slack of the eyes the server has for them, plus ExtraReach
	 * for a muzzle offset, and falls back to those eyes if geometry lies in between. False without a shooter.
	 */
	static bool ClampClientShotOrigin(const AActor* Shooter, FVector& InOutOrigin, float ExtraReach = 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Weapon|FX")
	FGameplayTag FireCueTag;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Weapon|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Weapon|Trace")
	float TraceRange = 10000.f;

	/** Fires FireCueTag with this weapon as the cue's SourceObject. Target data is optional. */
	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Weapon", meta = (AutoCreateRefTerm = "TargetData"))
	void ExecuteFireCue(const FGameplayAbilityTargetDataHandle& TargetData);

	bool GetWeaponViewpoint(FVector& OutLocation, FRotator& OutRotation) const;

	FCollisionQueryParams MakeWeaponTraceParams() const;

	bool ApplyEffectToTargetFromHit(TSubclassOf<UGameplayEffect> EffectClass, const FHitResult& Hit) const;

	/** Applies weapon damage, coalescing every hit on the same actor into a single effect.
	 *  Returns how many actors were damaged. */
	int32 ApplyDamageEffectToTargets(TArrayView<const FHitResult> Hits) const;
};
