// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "ASVitalsViewModel.generated.h"

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASVitalsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetHealth(float Value);
	void SetMaxHealth(float Value);
	void SetShield(float Value);
	void SetMaxShield(float Value);

	UFUNCTION(BlueprintPure, FieldNotify)
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	float GetShieldPercent() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	FText GetHealthText() const;
	
	UFUNCTION(BlueprintPure, FieldNotify)
	FText GetShieldText() const;

private:
	
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	float Health = 0.f;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	float MaxHealth = 0.f;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	float Shield = 0.f;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	float MaxShield = 0.f;
};
