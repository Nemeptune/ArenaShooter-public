// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ASInputConfig.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FASInputAction
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * 
 */
UCLASS(BlueprintType)
class ARENASHOOTER_API UASInputConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FASInputAction> AbilityInputActions;
};
