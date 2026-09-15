#pragma once
#include "GameSettingCollection.h"
#include "GameSettingFilterState.h"
#include "GameSettingValueDiscreteDynamic.h"
#include "DataSource/GameSettingDataSource.h"

#define LOCTEXT_NAMESPACE "ASSettings"

class FASWhenCondition : public FGameSettingEditCondition
{
public:
	using FFunc = TFunction<void(const ULocalPlayer*, FGameSettingEditableState&)>;
	explicit FASWhenCondition(FFunc InFunc) : Func(MoveTemp(InFunc)) {}
	virtual void GatherEditState(const ULocalPlayer* InLocalPlayer, FGameSettingEditableState& InOutEditState) const override
	{
		Func(InLocalPlayer, InOutEditState);
	}
private:
	FFunc Func;
};

static void AddQualityBucket(
	UGameSettingCollection* Parent, UGameSetting* Preset,
	const FName& DevName, const FText& DisplayName, const FText& Description,
	const TSharedRef<FGameSettingDataSource>& Getter,
	const TSharedRef<FGameSettingDataSource>& Setter)
{
	UGameSettingValueDiscreteDynamic_Number* Setting = NewObject<UGameSettingValueDiscreteDynamic_Number>();
	Setting->SetDevName(DevName);
	Setting->SetDisplayName(DisplayName);
	Setting->SetDescriptionRichText(Description);
	Setting->SetDynamicGetter(Getter);
	Setting->SetDynamicSetter(Setter);
	Setting->AddOption(0, LOCTEXT("Q_Low","Low"));
	Setting->AddOption(1, LOCTEXT("Q_Medium","Medium"));
	Setting->AddOption(2, LOCTEXT("Q_High","High"));
	Setting->AddOption(3, LOCTEXT("Q_Epic","Epic"));
	Setting->SetDefaultValue(1);

	Setting->AddEditDependency(Preset);
	Preset->AddEditDependency(Setting);

	Parent->AddSetting(Setting);
}

#undef LOCTEXT_NAMESPACE