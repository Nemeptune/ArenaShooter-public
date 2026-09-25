// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASWeaponInstance.h"
#include "ASWeaponInstance_RocketLauncher.generated.h"

class UAbilitySystemComponent;

UCLASS()
class ARENASHOOTER_API UASWeaponInstance_RocketLauncher : public UASWeaponInstance
{
	GENERATED_BODY()
	
protected:
	virtual void OnFired() override;
	virtual void OnEquipped() override;

private:
	void ApplyLoadedRoundVisibility(bool bLoaded);

	FTimerHandle ReloadVisualTimer;
};
