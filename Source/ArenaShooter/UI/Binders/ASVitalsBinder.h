// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASBinderBase.h"
#include "ASVitalsBinder.generated.h"

class UASVitalsViewModel;
class UASAbilitySystemComponent;
struct FOnAttributeChangeData;

UCLASS()
class ARENASHOOTER_API UASVitalsBinder : public UASBinderBase
{
	GENERATED_BODY()

public:
	void Init(UASAbilitySystemComponent* ASC, UASVitalsViewModel* InViewModel);

protected:
	virtual void SubscribeAll() override;
	virtual void UnSubscribeAll() override;
	virtual void RefreshAll() override;

private:
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);
	void HandleShieldChanged(const FOnAttributeChangeData& Data);
	void HandleMaxShieldChanged(const FOnAttributeChangeData& Data);
	
	TWeakObjectPtr<UASAbilitySystemComponent> BoundASC;
	UPROPERTY()
	TObjectPtr<UASVitalsViewModel> ViewModel;
};
