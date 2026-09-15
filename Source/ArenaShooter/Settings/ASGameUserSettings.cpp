// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameUserSettings.h"

#include "ASAudioSettingsSubsystem.h"

UASGameUserSettings::UASGameUserSettings()
{
	SetToDefaults();
}

UASGameUserSettings* UASGameUserSettings::GetASGameUserSettings()
{
	return GEngine ? Cast<UASGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UASGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	OverallVolume = 1.0f;
	SfxVolume = 1.0f;
	MusicVolume = 1.0f;
	UIVolume = 1.0f;
	LookSensitivity = 1.0f;
}

float UASGameUserSettings::FrameCapToFloat(EASFrameCap Value)
{
	switch (Value)
	{
	case EASFrameCap::FPS30:
		return 30.f;
	case EASFrameCap::FPS60:
		return 60.f;
	case EASFrameCap::FPS120:
		return 120.f;
	case EASFrameCap::FPS144:
		return 144.f;
	default:
		return 0.f;
	}
}

void UASGameUserSettings::RefreshAudioBuses()
{
	if (!GEngine)
	{
		return;
	}

	if (UWorld* World = GEngine->GetCurrentPlayWorld())
	{
		if (UASAudioSettingsSubsystem* Audio = World->GetSubsystem<UASAudioSettingsSubsystem>())
		{
			Audio->ApplyAll();
		}
	}
}

void UASGameUserSettings::SetOverallVolume(float Value)
{
	OverallVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	RefreshAudioBuses();
}

void UASGameUserSettings::SetSfxVolume(float Value)
{
	SfxVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	RefreshAudioBuses();
}

void UASGameUserSettings::SetMusicVolume(float Value)
{
	MusicVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	RefreshAudioBuses();
}

void UASGameUserSettings::SetUIVolume(float Value)
{
	UIVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	RefreshAudioBuses();
}

void UASGameUserSettings::SetLookSensitivity(float Value)
{
	LookSensitivity = FMath::Clamp(Value, 0.0f, 3.0f);
}

float UASGameUserSettings::GetOverallVolume() const
{
	return OverallVolume;
}

float UASGameUserSettings::GetSfxVolume() const
{
	return SfxVolume;
}

float UASGameUserSettings::GetMusicVolume() const
{
	return MusicVolume;
}

float UASGameUserSettings::GetUIVolume() const
{
	return UIVolume;
}

float UASGameUserSettings::GetLookSensitivity() const
{
	return LookSensitivity;
}

bool UASGameUserSettings::GetVSync() const
{
	return IsVSyncEnabled();
}

void UASGameUserSettings::SetVSync(bool Value)
{
	SetVSyncEnabled(Value);
}

EASGraphicsQuality UASGameUserSettings::GetOverallQualityEnum() const
{
	// TODO stop clamping once detailed settings implemented
	return static_cast<EASGraphicsQuality>(FMath::Clamp(GetOverallScalabilityLevel(), 0, 3));
}

void UASGameUserSettings::SetOverallQualityEnum(EASGraphicsQuality Value)
{
	SetOverallScalabilityLevel(static_cast<int32>(Value));
}

EASWindowMode UASGameUserSettings::GetWindowModeEnum() const
{
	switch (GetFullscreenMode())
	{
	case EWindowMode::Fullscreen:
		return EASWindowMode::Fullscreen;
	case EWindowMode::WindowedFullscreen:
		return EASWindowMode::Borderless;
	default:
		return EASWindowMode::Windowed;
	}
}

void UASGameUserSettings::SetWindowModeEnum(EASWindowMode Mode)
{
	switch (Mode)
	{
	case EASWindowMode::Fullscreen:
		 SetFullscreenMode(EWindowMode::Fullscreen);
		break;
	case  EASWindowMode::Borderless:
		SetFullscreenMode(EWindowMode::WindowedFullscreen);
		break;
	default:
		SetFullscreenMode(EWindowMode::Windowed);
		break;
	}
}

EASFrameCap UASGameUserSettings::GetFrameCap() const
{
	const float Value = GetFrameRateLimit();
	if (FMath::IsNearlyEqual(Value, 30.f))
	{
		return EASFrameCap::FPS30;
	}
	if (FMath::IsNearlyEqual(Value, 60.f))
	{
		return EASFrameCap::FPS60;
	}
	if (FMath::IsNearlyEqual(Value, 120.f))
	{
		return EASFrameCap::FPS120;
	}
	if (FMath::IsNearlyEqual(Value, 144.f))
	{
		return EASFrameCap::FPS144;
	}

	return EASFrameCap::Unlimited;
}

void UASGameUserSettings::SetFrameCap(EASFrameCap Value)
{
	SetFrameRateLimit(FrameCapToFloat(Value));
}
