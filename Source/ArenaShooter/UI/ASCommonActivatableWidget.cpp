// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ASCommonActivatableWidget.h"

TOptional<FUIInputConfig> UASCommonActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputConfig)
	{
	case EASWidgetInputMode::GameAndMenu:
		return FUIInputConfig(ECommonInputMode::All, GameMouseCaptureMode);
		
	case EASWidgetInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode);
		
	case EASWidgetInputMode::Menu:
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
		
	case EASWidgetInputMode::Default:
	default:
		return TOptional<FUIInputConfig>();
	}
}
