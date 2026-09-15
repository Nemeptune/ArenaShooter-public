// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "ASLocalPlayer.generated.h"

class UASUIConfig;
class UASGameUserSettings;
class UASGameSettingRegistry;
/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

public:
	UFUNCTION()
	UASGameUserSettings* GetLocalSettings();

	UASGameSettingRegistry* GetSettingsRegistry();

	UASUIConfig* GetUIConfig();
	
	DECLARE_MULTICAST_DELEGATE_TwoParams(FASPlayerControllerSet, UASLocalPlayer*, APlayerController*);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FASPlayerStateSet, UASLocalPlayer*, APlayerState*);
	
	FASPlayerControllerSet OnPlayerControllerSet;
	FASPlayerStateSet OnPlayerStateSet;
	
	FDelegateHandle CallAndRegister_OnPlayerControllerSet(FASPlayerControllerSet::FDelegate Delegate);
	FDelegateHandle CallAndRegister_OnPlayerStateSet(FASPlayerStateSet::FDelegate Delegate);

private:
	UPROPERTY(Transient)
	TObjectPtr<UASGameSettingRegistry> SettingsRegistry;

	UPROPERTY(Transient)
	TObjectPtr<UASUIConfig> UIConfig;
};
