#pragma once
#include "GameSettingValueDiscrete.h"
#include "UASSettingValueDiscrete_OverallQuality.generated.h"

UCLASS()
class ARENASHOOTER_API UASSettingValueDiscrete_OverallQuality : public UGameSettingValueDiscrete
{
	GENERATED_BODY()
public:
	UASSettingValueDiscrete_OverallQuality();

	virtual void StoreInitial() override;
	virtual void ResetToDefault() override;
	virtual void RestoreToInitial() override;

	virtual void SetDiscreteOptionByIndex(int32 Index) override;
	virtual int32 GetDiscreteOptionIndex() const override;
	virtual TArray<FText> GetDiscreteOptions() const override;

protected:
	virtual void OnInitialized() override;
	virtual void OnDependencyChanged() override;

	int32 GetCustomIndex() const;
	int32 GetOverallQualilyLevel() const;
	TArray<FText> Options;
	TArray<FText> OptionsWithCustom; // user should not be able to select custom while choosing preset
};
