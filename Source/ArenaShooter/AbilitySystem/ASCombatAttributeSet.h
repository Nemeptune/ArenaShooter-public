// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ASAttributeSet.h"
#include "ASCombatAttributeSet.generated.h"

struct FGameplayEffectModCallbackData;

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASCombatAttributeSet : public UASAttributeSet
{
	GENERATED_BODY()
	
public:
	
	UASCombatAttributeSet();

	ATTRIBUTE_ACCESSORS(UASCombatAttributeSet, Health);
	ATTRIBUTE_ACCESSORS(UASCombatAttributeSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UASCombatAttributeSet, Shield);
	ATTRIBUTE_ACCESSORS(UASCombatAttributeSet, MaxShield);
	ATTRIBUTE_ACCESSORS(UASCombatAttributeSet, Healing);
	ATTRIBUTE_ACCESSORS(UASCombatAttributeSet, Damage);
	
	ATTRIBUTE_ACCESSORS(UASCombatAttributeSet, DamageMultiplier);
	ATTRIBUTE_ACCESSORS(UASCombatAttributeSet, DamageResistance);
	
	UFUNCTION(BlueprintCallable, Category = "CombatAttributes")
	float ASGetHealth() const;
	UFUNCTION(BlueprintCallable, Category = "CombatAttributes")
	void ASSetHealth(float NewValue);
	UFUNCTION(BlueprintCallable, Category = "CombatAttributes")
	void ASInitHealth(float NewValue);
	UFUNCTION(BlueprintCallable, Category = "CombatAttributes")
	void ASInitMaxHealth(float NewValue);
	UFUNCTION(BlueprintCallable, Category = "CombatAttributes")
	void ASInitShield(float NewValue);

protected:

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldValue);

	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData &Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = "OnRep_Health", Category = "Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = "OnRep_MaxHealth", Category = "Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = "OnRep_Shield", Category = "Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Shield;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = "OnRep_MaxShield", Category = "Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxShield;
	
	UPROPERTY(BlueprintReadOnly, Category="Buffs", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData DamageMultiplier;

	UPROPERTY(BlueprintReadOnly, Category="Buffs", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData DamageResistance;

	float ShieldAbsorptionPercent;

	// -------------------------------------------------------------------
	//	Meta Attribute 
	// -------------------------------------------------------------------
	
	// Incoming healing. This is mapped directly to +Health
	UPROPERTY(BlueprintReadOnly, Category="Health", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Healing;

	// Incoming damage. This is mapped directly to -Health
	UPROPERTY(BlueprintReadOnly, Category="Health", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Damage;
	
	float ShieldDamageThisExecution = 0.f;

	void BroadcastDamageDealt(const FGameplayEffectModCallbackData& Data, AActor* TargetActor, float TotalDamage, bool bLethal) const;
	void ApplyDamageReactions(const FGameplayEffectModCallbackData& Data, float TotalDealt) const;
};
