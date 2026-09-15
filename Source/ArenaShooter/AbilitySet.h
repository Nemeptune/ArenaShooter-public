// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayEffectTypes.h"
#include "AbilitySet.generated.h"

class UASAbilitySystemComponent;
class UGameplayEffect;
class UASGameplayAbility;

USTRUCT(BlueprintType)
struct FAbilitySet_GameplayAbility
{
	GENERATED_BODY()
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AbilitySet")
	TSubclassOf<UASGameplayAbility> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly)
	int32 AbilityLevel = 1;
};

USTRUCT(BlueprintType)
struct FAbilitySet_GameplayEffect
{
	GENERATED_BODY()

public:

	// Gameplay effect to grant.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

	// Level of gameplay effect to grant.
	UPROPERTY(EditDefaultsOnly)
	float EffectLevel = 1.0f;
};

USTRUCT(BlueprintType)
struct FAbilitySet_GrantedHandles
{
	GENERATED_BODY()
public:

	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);

	void TakeFromAbilitySystem(UASAbilitySystemComponent* ASC);
	

protected:
	
	UPROPERTY(VisibleAnywhere, Category="AbilitySet")
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY(VisibleAnywhere, Category="AbilitySet")
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;
};

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UAbilitySet : public UDataAsset
{
	GENERATED_BODY()
	
public:
	
	void GiveToAbilitySystem(UASAbilitySystemComponent* ASC, FAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;
	
protected:
	// Gameplay abilities to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta=(TitleProperty=Ability))
	TArray<FAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	// Gameplay effects to grant when this ability set is granted.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
	TArray<FAbilitySet_GameplayEffect> GrantedGameplayEffects;
};
