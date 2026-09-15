// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Components/SlateWrapperTypes.h"
#include "ASWeaponSlotViewModel.generated.h"

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASWeaponSlotViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
public:
	void SetAmmo(int32 Value);
	void SetIsActive(bool Value);
	void SetIcon(const FSlateBrush& Value);
	UFUNCTION(BlueprintPure, FieldNotify)
	FSlateBrush GetIcon() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	FText GetAmmoText() const;

	UFUNCTION(BlueprintPure, FieldNotify)
	ESlateVisibility GetActiveHighlightVisibility() const;
	UFUNCTION(BlueprintPure, FieldNotify)
	ESlateVisibility GetSlotVisibility() const;
private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	int32 Ammo = 0;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	FSlateBrush Icon;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	bool bIsActive = false;
};
