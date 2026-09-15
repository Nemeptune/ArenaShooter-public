// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ASUISettings.generated.h"

class UNiagaraSystem;
class UASUIConfig;
/**
 * Project-wide UI configuration.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Arena Shooter UI"))
class ARENASHOOTER_API UASUISettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TSoftObjectPtr<UASUIConfig> UIConfig;
	
	UPROPERTY(Config, EditAnywhere, Category = "Number Pops")
	TSoftObjectPtr<UNiagaraSystem> NumberPopSystem;

	/** Vector4 array parameter on that system: XYZ = world position, W = the number. */
	UPROPERTY(Config, EditAnywhere, Category = "Number Pops")
	FName NiagaraArrayName = FName("DamageInfo");
	
	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TSoftObjectPtr<UTexture2D> DefaultAvatar;
};
