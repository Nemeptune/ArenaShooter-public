// Fill out your copyright notice in the Description page of Project Settings.


#include "ASHUDWidget.h"

#include "ASUISettings.h"

FSlateBrush UASHUDWidget::MakeAvatarBrush(UTexture2D* Avatar) const
{
	UTexture2D* Texture = Avatar;
	if (!Texture)
	{
		Texture = GetDefault<UASUISettings>()->DefaultAvatar.LoadSynchronous();
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.SetImageSize(FVector2D(64.f, 64.f));
	Brush.DrawAs = Texture ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
	return Brush;
}

TOptional<FUIInputConfig> UASHUDWidget::GetDesiredInputConfig() const
{
	
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently);
}
