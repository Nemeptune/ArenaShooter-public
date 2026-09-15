// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/GameSettingScreen.h"
#include "ASSettingsScreen.generated.h"

class UGameSettingRegistry;
class UGameSettingPanel;
/**
 * 
 */
UCLASS(Abstract)
class ARENASHOOTER_API UASSettingsScreen : public UGameSettingScreen
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnDeactivated() override;
	
	virtual UGameSettingRegistry* CreateRegistry() override;

	void HandleBackAction();

	UPROPERTY(EditDefaultsOnly)
	FDataTableRowHandle BackInputActionData;
	
	//FUIActionBindingHandle BackHandle;
};
