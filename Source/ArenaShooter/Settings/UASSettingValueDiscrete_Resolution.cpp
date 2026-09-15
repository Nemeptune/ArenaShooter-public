#include "UASSettingValueDiscrete_Resolution.h"

#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

UASSettingValueDiscrete_Resolution::UASSettingValueDiscrete_Resolution()
{
}

void UASSettingValueDiscrete_Resolution::OnInitialized()
{
	Super::OnInitialized();
	RebuildResolutionOptions();
}

void UASSettingValueDiscrete_Resolution::RebuildResolutionOptions()
{
	Resolutions.Reset();
	Options.Reset();

	const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	const EWindowMode::Type Mode = Settings->GetFullscreenMode();

	if (Mode == EWindowMode::WindowedFullscreen)
	{
		Resolutions.Add(Settings->GetScreenResolution());
	}
	else if (Mode == EWindowMode::Fullscreen)
	{
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	}
	else
	{
		UKismetSystemLibrary::GetConvenientWindowedResolutions(Resolutions);
	}

	const FIntPoint Current = Settings->GetScreenResolution();
	if (!Resolutions.Contains(Current))
	{
		Resolutions.Add(Current);
	}

	for (const FIntPoint& Res : Resolutions)
	{
		Options.Add(FText::FromString(FString::Printf(TEXT("%d x %d"), Res.X, Res.Y)));
	}
}

void UASSettingValueDiscrete_Resolution::StoreInitial()
{
	// Ignore
}

void UASSettingValueDiscrete_Resolution::ResetToDefault()
{
	// Ignore
}

void UASSettingValueDiscrete_Resolution::RestoreToInitial()
{
	// Ignore
}

void UASSettingValueDiscrete_Resolution::SetDiscreteOptionByIndex(int32 Index)
{
	if (!Resolutions.IsValidIndex(Index))
	{
		return;
	}
	UGameUserSettings::GetGameUserSettings()->SetScreenResolution(Resolutions[Index]);
	NotifySettingChanged(EGameSettingChangeReason::Change);
}

int32 UASSettingValueDiscrete_Resolution::GetDiscreteOptionIndex() const
{
	const FIntPoint Current = UGameUserSettings::GetGameUserSettings()->GetScreenResolution();
	const int32 Index = Resolutions.IndexOfByKey(Current);
	return Index != INDEX_NONE ? Index : 0;
}

TArray<FText> UASSettingValueDiscrete_Resolution::GetDiscreteOptions() const
{
	return Options;
}

void UASSettingValueDiscrete_Resolution::OnDependencyChanged()
{
	RebuildResolutionOptions();
	NotifySettingChanged(EGameSettingChangeReason::Change);
}
