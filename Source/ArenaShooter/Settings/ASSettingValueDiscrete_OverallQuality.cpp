#include "ASSettingValueDiscrete_OverallQuality.h"

#include "ASGameUserSettings.h"

#define LOCTEXT_NAMESPACE "ASSettings"

UASSettingValueDiscrete_OverallQuality::UASSettingValueDiscrete_OverallQuality()
{
}

void UASSettingValueDiscrete_OverallQuality::OnInitialized()
{
	Super::OnInitialized();

	Options = {
		LOCTEXT("OQ_Low","Low"),
		LOCTEXT("OQ_Med","Medium"),
		LOCTEXT("OQ_High","High"),
		LOCTEXT("OQ_Epic","Epic"),
	};
	OptionsWithCustom = Options;
	OptionsWithCustom.Add(LOCTEXT("OQ_Custom","Custom"));
}

int32 UASSettingValueDiscrete_OverallQuality::GetDiscreteOptionIndex() const
{
	const int32 Level = GetOverallQualilyLevel();
	if (Level == INDEX_NONE)
	{
		return GetCustomIndex();
	}
	return Level;
}

void UASSettingValueDiscrete_OverallQuality::SetDiscreteOptionByIndex(int32 Index)
{
	if (Index == GetCustomIndex())
	{
		return;
	}
	UASGameUserSettings::GetASGameUserSettings()->SetOverallScalabilityLevel(Index);
	NotifySettingChanged(EGameSettingChangeReason::Change);
}

void UASSettingValueDiscrete_OverallQuality::OnDependencyChanged()
{
	NotifySettingChanged(EGameSettingChangeReason::Change);
}

void UASSettingValueDiscrete_OverallQuality::StoreInitial()
{
}

void UASSettingValueDiscrete_OverallQuality::ResetToDefault()
{
	//UASGameUserSettings::GetASGameUserSettings()->SetOverallScalabilityLevel(3); // wrong
}

void UASSettingValueDiscrete_OverallQuality::RestoreToInitial()
{
}

TArray<FText> UASSettingValueDiscrete_OverallQuality::GetDiscreteOptions() const
{
	const int32 Level = GetOverallQualilyLevel();
	if (Level == INDEX_NONE)
	{
		return OptionsWithCustom;
	}
	else
	{
		return Options;
	}
}

int32 UASSettingValueDiscrete_OverallQuality::GetCustomIndex() const
{
	return OptionsWithCustom.Num() - 1;
}

int32 UASSettingValueDiscrete_OverallQuality::GetOverallQualilyLevel() const
{
	const UGameUserSettings* UserSettings = CastChecked<const UGameUserSettings>(GEngine->GetGameUserSettings());
	return UserSettings->GetOverallScalabilityLevel();
}

#undef LOCTEXT_NAMESPACE
