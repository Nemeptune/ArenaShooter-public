// Fill out your copyright notice in the Description page of Project Settings.


#include "ASLocalPlayer.h"
#include "Settings/ASGameSettingRegistry.h"
#include "Settings/ASGameUserSettings.h"
#include "UI/ASUISettings.h"
#include "UI/ASUIConfig.h"

UASGameUserSettings* UASLocalPlayer::GetLocalSettings()
{
	return UASGameUserSettings::GetASGameUserSettings();
}

UASGameSettingRegistry* UASLocalPlayer::GetSettingsRegistry()
{
	if (!SettingsRegistry)
	{
		SettingsRegistry = NewObject<UASGameSettingRegistry>(this);
		SettingsRegistry->Initialize(this);
	}
	
	return SettingsRegistry;
}

UASUIConfig* UASLocalPlayer::GetUIConfig()
{
	if (!UIConfig)
	{
		if (const UASUISettings* Settings = GetDefault<UASUISettings>())
		{
			UIConfig = Settings->UIConfig.LoadSynchronous();
		}
	}
	
	return UIConfig;
}

FDelegateHandle UASLocalPlayer::CallAndRegister_OnPlayerControllerSet(FASPlayerControllerSet::FDelegate Delegate)
{
	if (APlayerController* PC = GetPlayerController(GetWorld()))
	{
		Delegate.Execute(this, PC);
	}
	
	return OnPlayerControllerSet.Add(Delegate);
}

FDelegateHandle UASLocalPlayer::CallAndRegister_OnPlayerStateSet(FASPlayerStateSet::FDelegate Delegate)
{
	APlayerController* PC = GetPlayerController(GetWorld());
	if (APlayerState* PS = PC ? PC->PlayerState : nullptr)
	{
		Delegate.Execute(this, PS);
	}
	return OnPlayerStateSet.Add(Delegate);
}
