// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "ASAmmoReceiver.generated.h"

UINTERFACE()
class UASAmmoReceiver : public UInterface
{
	GENERATED_BODY()
};


class ARENASHOOTER_API IASAmmoReceiver
{
	GENERATED_BODY()

public:
	virtual int32 GiveAmmo(FGameplayTag AmmoType, int32 Amount) = 0;
};
