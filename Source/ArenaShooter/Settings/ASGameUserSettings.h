// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "ASGameUserSettings.generated.h"

UENUM()
enum class EASGraphicsQuality : uint8
{
	Low,
	Medium,
	High,
	Epic
};

UENUM()
enum class EASWindowMode : uint8
{
	Fullscreen,
	Borderless,
	Windowed
};

UENUM()
enum class EASFrameCap : uint8
{
	FPS30,
	FPS60,
	FPS120,
	FPS144,
	Unlimited
};

/**
 * 
 */
UCLASS(Config = GameUserSettings)
class ARENASHOOTER_API UASGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	UASGameUserSettings();

	UFUNCTION(BlueprintPure, Category = "Settings")
	static UASGameUserSettings* GetASGameUserSettings();

	// --- Audio ---
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetOverallVolume(float Value);
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	float GetOverallVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSfxVolume(float Value);
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	float GetSfxVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float Value);
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetUIVolume(float Value);
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	float GetUIVolume() const;

	// --- Controls ---
	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetLookSensitivity(float Value);
	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	float GetLookSensitivity() const;

	// --- Video ---
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	bool GetVSync() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetVSync(bool Value);

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	EASGraphicsQuality GetOverallQualityEnum() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetOverallQualityEnum(EASGraphicsQuality Value);

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	EASWindowMode GetWindowModeEnum() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetWindowModeEnum(EASWindowMode Mode);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	EASFrameCap GetFrameCap() const;
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetFrameCap(EASFrameCap Value);

	virtual void SetToDefaults() override;

	static float FrameCapToFloat(EASFrameCap Value);

protected:
	UPROPERTY(Config)
	float OverallVolume;
	UPROPERTY(Config)
	float SfxVolume;
	UPROPERTY(Config)
	float MusicVolume;
	UPROPERTY(Config)
	float UIVolume;
	UPROPERTY(Config)
	float LookSensitivity;

private:
	void RefreshAudioBuses();
};
