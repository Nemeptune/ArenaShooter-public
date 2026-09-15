// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SoundControlBusMix.h"
#include "SoundControlBus.h"
#include "ASAudioSettings.generated.h"

class UMetaSoundSource;

USTRUCT()
struct FASMetaSoundPrecache
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Precache")
	TSoftObjectPtr<UMetaSoundSource> Sound;

	/** Copies built in advance = how many of this sound can overlap and still start instantly. */
	UPROPERTY(EditAnywhere, Category="Precache", meta=(ClampMin=1, ClampMax=8))
	int32 Instances = 2;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName= "AS Audio Settings"))
class ARENASHOOTER_API UASAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category="Audio Settings")
	TSoftObjectPtr<USoundControlBusMix> UserMix;
	UPROPERTY(Config, EditAnywhere, Category="Audio Settings")
	TSoftObjectPtr<USoundControlBus> OverallBus;
	UPROPERTY(Config, EditAnywhere, Category="Audio Settings")
	TSoftObjectPtr<USoundControlBus> SfxBus;
	UPROPERTY(Config, EditAnywhere, Category="Audio Settings")
	TSoftObjectPtr<USoundControlBus> MusicBus;
	UPROPERTY(Config, EditAnywhere, Category="Audio Settings")
	TSoftObjectPtr<USoundControlBus> UIBus;
	
	UPROPERTY(Config, EditAnywhere, Category="Precache")
	TArray<FASMetaSoundPrecache> PrecacheMetaSounds;
};
