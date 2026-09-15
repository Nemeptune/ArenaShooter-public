// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ASAudioSettingsSubsystem.generated.h"

class UMetaSoundSource;
class USoundControlBusMix;
class USoundControlBus;

UCLASS()
class ARENASHOOTER_API UASAudioSettingsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	UFUNCTION(BlueprintCallable)
	void ApplyAll();

private:
	void LoadReferences();

	UPROPERTY()
	TObjectPtr<USoundControlBusMix> Mix;
	UPROPERTY()
	TObjectPtr<USoundControlBus> OverallBus;
	UPROPERTY()
	TObjectPtr<USoundControlBus> SfxBus;
	UPROPERTY()
	TObjectPtr<USoundControlBus> MusicBus;
	UPROPERTY()
	TObjectPtr<USoundControlBus> UIBus;

	bool bMixActivated = false;
};
