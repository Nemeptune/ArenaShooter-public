// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameSettingRegistry.h"
#include "ASGameSettingRegistry.generated.h"

class UASLocalPlayer;

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASGameSettingRegistry : public UGameSettingRegistry
{
	GENERATED_BODY()

public:
	virtual void SaveChanges() override;

protected:
	virtual void OnInitialize(ULocalPlayer* InLocalPlayer) override;

	UGameSettingCollection* InitializeAudioSettings(UASLocalPlayer* InLocalPlayer);
	UGameSettingCollection* InitializeVideoSettings(UASLocalPlayer* InLocalPlayer);
	UGameSettingCollection* InitializeControlSettings(UASLocalPlayer* InLocalPlayer);

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> ControlSettings;
	UPROPERTY()
	TObjectPtr<UGameSettingCollection> AudioSettings;
	UPROPERTY()
	TObjectPtr<UGameSettingCollection> VideoSettings;
};
