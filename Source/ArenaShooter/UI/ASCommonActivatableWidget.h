// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "ASCommonActivatableWidget.generated.h"

UENUM()
enum class EASWidgetInputMode
{
	Default,
	GameAndMenu,
	Game,
	Menu
};

/**
 * Base activatable widget — pick the input mode per widget in the Details panel.
 */
UCLASS(Abstract)
class ARENASHOOTER_API UASCommonActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()
	
protected:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	EASWidgetInputMode InputConfig = EASWidgetInputMode::Default;

	UPROPERTY(EditAnywhere, Category = "Input")
	EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
};
