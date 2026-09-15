// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ASGameplayAbility.generated.h"

class AASCharacter;
class AASPlayerController;
class UASAbilitySystemComponent;
class UASInventoryComponent;

/**
 *	Defines how an ability is meant to activate.
 */
UENUM(BlueprintType)
enum class EAbilityActivationPolicy : uint8
{
	// Try to activate the ability when the input is triggered.
	OnInputTriggered,

	// Continually try to activate the ability while the input is active.
	WhileInputActive,

	// Try to activate the ability when an avatar is assigned.
	OnSpawn
};

USTRUCT()
struct ARENASHOOTER_API FAbilityMeshMontage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;

	FAbilityMeshMontage() : Mesh(nullptr), Montage(nullptr)
	{
	}

	FAbilityMeshMontage(class USkeletalMeshComponent* InMesh, class UAnimMontage* InMontage) 
		: Mesh(InMesh), Montage(InMontage)
	{
	}
};

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UASGameplayAbility();

	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Ability")
	UASAbilitySystemComponent* GetASAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Ability")
	AASPlayerController* GetASPlayerControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Ability")
	AController* GetControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Ability")
	AASCharacter* GetASCharacterFromActorInfo() const;
	
	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Ability")
	UASInventoryComponent* GetInventoryFromActorInfo() const;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ArenaShooter|Input")
	FGameplayTag InputTag;
	
	// If an ability has EAbilityActivationPolicy::OnSpawn, activate them immediately when given here
	// Epic's comment: Projects may want to initiate passives or do other "BeginPlay" type of logic here.
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// ----------------------------------------------------------------------------------------------------------------
	//	Animation Support for multiple USkeletalMeshComponents on the AvatarActor
	// ----------------------------------------------------------------------------------------------------------------

	/** Returns the currently playing montage for this ability, if any */
	UFUNCTION(BlueprintCallable, Category = Animation)
	UAnimMontage* GetCurrentMontageForMesh(USkeletalMeshComponent* InMesh);

	/** Call to set/get the current montage from a montage task. Set to allow hooking up montage events to ability events */
	virtual void SetCurrentMontageForMesh(USkeletalMeshComponent* InMesh, class UAnimMontage* InCurrentMontage);
	
protected:
	
	// Defines how this ability is meant to activate.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Ability Activation")
	EAbilityActivationPolicy ActivationPolicy;

	/** Active montages being played by this ability */
	UPROPERTY()
	TArray<FAbilityMeshMontage> CurrentAbilityMeshMontages;

	bool FindAbillityMeshMontage(USkeletalMeshComponent* InMesh, FAbilityMeshMontage& InAbilityMontage);
	
};
