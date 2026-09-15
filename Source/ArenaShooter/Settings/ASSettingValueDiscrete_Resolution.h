#pragma once
#include "GameSettingValueDiscrete.h"
#include "ASSettingValueDiscrete_Resolution.generated.h"

UCLASS()
class UASSettingValueDiscrete_Resolution : public UGameSettingValueDiscrete
{

	GENERATED_BODY()
	
public:
	UASSettingValueDiscrete_Resolution();

	virtual void StoreInitial() override;
	virtual void ResetToDefault() override;
	virtual void RestoreToInitial() override;

	virtual void SetDiscreteOptionByIndex(int32 Index) override;
	virtual int32 GetDiscreteOptionIndex() const override;
	virtual TArray<FText> GetDiscreteOptions() const override;

protected:
	virtual void OnInitialized() override;
	virtual void OnDependencyChanged() override;

	void RebuildResolutionOptions();

private:
	TArray<FIntPoint> Resolutions;
	TArray<FText> Options;
	FIntPoint InitialResolution = FIntPoint(0, 0);
};
