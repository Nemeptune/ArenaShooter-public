// Fill out your copyright notice in the Description page of Project Settings.


#include "Settings/ASAudioSettingsSubsystem.h"

#include "ASAudioSettings.h"
#include "ASGameUserSettings.h"
#include "AudioModulationStatics.h"
#include "MetasoundOperatorCacheSubsystem.h"
#include "AudioDevice.h"
#include "MetasoundSource.h"

bool UASAudioSettingsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// Only real game worlds
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

void UASAudioSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadReferences();
}

void UASAudioSettingsSubsystem::LoadReferences()
{
	const auto* Settings = GetDefault<UASAudioSettings>();
	
	Mix = Settings->UserMix.LoadSynchronous();
	OverallBus = Settings->OverallBus.LoadSynchronous();
	SfxBus = Settings->SfxBus.LoadSynchronous();
	MusicBus = Settings->MusicBus.LoadSynchronous();
	UIBus = Settings->UIBus.LoadSynchronous();
}

void UASAudioSettingsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	
	if (FAudioDevice* AudioDevice = InWorld.GetAudioDeviceRaw())
	{
		if (UMetaSoundCacheSubsystem* Cache = AudioDevice->GetSubsystem<UMetaSoundCacheSubsystem>())
		{
			for (const FASMetaSoundPrecache& Entry : GetDefault<UASAudioSettings>()->PrecacheMetaSounds)
			{
				if (UMetaSoundSource* Source = Entry.Sound.LoadSynchronous())
				{
					Cache->TouchOrPrecacheMetaSound(Source, Entry.Instances);
				}
			}
		}
	}

	if (Mix)
	{
		UAudioModulationStatics::ActivateBusMix(&InWorld, Mix);
		bMixActivated = true;
	}

	ApplyAll();
}

void UASAudioSettingsSubsystem::ApplyAll()
{
	UWorld* World = GetWorld();
	const auto GUS = UASGameUserSettings::GetASGameUserSettings();
	if (!World || !GUS || !Mix || !bMixActivated) return;

	TArray<FSoundControlBusMixStage> Stages;
	auto Add = [&](USoundControlBus* Bus, float Value)
	{
		if (!Bus) return;
		FSoundControlBusMixStage S;
		S.Bus = Bus;
		S.Value.TargetValue = Value;
		S.Value.AttackTime  = 0.01f;
		S.Value.ReleaseTime = 0.01f;
		Stages.Add(S);
	};
	
	Add(OverallBus, GUS->GetOverallVolume());
	Add(SfxBus,     GUS->GetSfxVolume());
	Add(MusicBus,   GUS->GetMusicVolume());
	Add(UIBus,      GUS->GetUIVolume());

	UAudioModulationStatics::UpdateMix(World, Mix, Stages);
}
