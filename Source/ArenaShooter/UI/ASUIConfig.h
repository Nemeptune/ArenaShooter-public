// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ASUIConfig.generated.h"

class UCommonActivatableWidget;
class UASResultWidget;
class UASHUDWidget;
class UASRootLayout;
/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASUIConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "Layout")
	TSoftClassPtr<UASRootLayout> RootLayoutClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSoftClassPtr<UASHUDWidget> HUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSoftClassPtr<UASResultWidget> ResultWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSoftClassPtr<UCommonActivatableWidget> GameMenuClass;
};
