// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ASBuffDefinition.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType, Const)
class ARENASHOOTER_API UASBuffDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff")
	TSubclassOf<UGameplayEffect> BuffEffect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<UStaticMesh> PickupMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	FLinearColor Color = FLinearColor::Blue;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	FLinearColor ColorSecondary = FLinearColor::White;
};
