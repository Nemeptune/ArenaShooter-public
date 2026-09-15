// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ASRootLayout.generated.h"

class UCommonActivatableWidgetStack;

/**
 * Full-screen container holding layers.
 * Catches the global Back/Esc key and forwards it to the layer coordinator.
 */
UCLASS(Abstract, Blueprintable)
class ARENASHOOTER_API UASRootLayout : public UUserWidget
{
	GENERATED_BODY()

protected:
	
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameStack;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameMenuStack;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> MenuStack;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> ModalStack;
};
