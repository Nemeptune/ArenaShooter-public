// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASCommonActivatableWidget.h"
#include "Styling/SlateBrush.h"
#include "ASHUDWidget.generated.h"

class UASWeaponSlotViewModel;

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASHUDWidget : public UASCommonActivatableWidget
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void InitWeaponSlots(const TArray<UASWeaponSlotViewModel*>& Slots);
	
	UFUNCTION(BlueprintPure, Category = "ASUI")
	FSlateBrush MakeAvatarBrush(UTexture2D* Avatar) const;
	
protected:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
};
