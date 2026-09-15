// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameSettingRegistry.h"

#include "ASGameUserSettings.h"
#include "Player/ASLocalPlayer.h"
#include "ASWhenCondition.h"
#include "GameSettingCollection.h"
#include "GameSettingValueDiscreteDynamic.h"
#include "GameSettingValueScalarDynamic.h"
#include "ASSettingValueDiscrete_OverallQuality.h"
#include "ASSettingValueDiscrete_Resolution.h"
#include "DataSource/GameSettingDataSourceDynamic.h"

#define GET_LOCAL_SETTINGS_FUNCTION_PATH(FunctionOrPropertyName)						\
	MakeShared<FGameSettingDataSourceDynamic>(TArray<FString>({							\
		GET_FUNCTION_NAME_STRING_CHECKED(UASLocalPlayer, GetLocalSettings),				\
		GET_FUNCTION_NAME_STRING_CHECKED(UASGameUserSettings, FunctionOrPropertyName)	\
}))

#define LOCTEXT_NAMESPACE "ASSettings"

void UASGameSettingRegistry::OnInitialize(ULocalPlayer* InLocalPlayer)
{
	UASLocalPlayer* LP = Cast<UASLocalPlayer>(InLocalPlayer);

	AudioSettings = InitializeAudioSettings(LP);
	RegisterSetting(AudioSettings);

	VideoSettings = InitializeVideoSettings(LP);
	RegisterSetting(VideoSettings);

	ControlSettings = InitializeControlSettings(LP);
	RegisterSetting(ControlSettings);
}

UGameSettingCollection* UASGameSettingRegistry::InitializeAudioSettings(UASLocalPlayer* InLocalPlayer)
{
	UGameSettingCollection* Screen = NewObject<UGameSettingCollection>();
	Screen->SetDevName(TEXT("AudioCollection"));
	Screen->SetDisplayName(LOCTEXT("AudioCollection_Name", "Audio"));
	Screen->Initialize(InLocalPlayer);

	// Overall Volume
	{
		UGameSettingValueScalarDynamic* Setting = NewObject<UGameSettingValueScalarDynamic>();
		Setting->SetDevName(TEXT("OverallVolume"));
		Setting->SetDisplayName(LOCTEXT("OverallVolume_Name","Overall Volume"));
		Setting->SetDescriptionRichText(LOCTEXT("OverallVolume_Description", "Adjusts the volume of everything."));
		
		Setting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetOverallVolume));
		Setting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetOverallVolume));
		Setting->SetDefaultValue(1.0f);
		Setting->SetDisplayFormat(UGameSettingValueScalarDynamic::ZeroToOnePercent);
		Setting->SetSourceRangeAndStep(TRange<double>(0.0,1.0), 0.01);
		Screen->AddSetting(Setting);
	}

	// SFX volume
	{
		UGameSettingValueScalarDynamic* Setting = NewObject<UGameSettingValueScalarDynamic>();
		Setting->SetDevName(TEXT("SfxVolume"));
		Setting->SetDisplayName(LOCTEXT("SfxVolume_Name","Effects Volume"));
		Setting->SetDescriptionRichText(LOCTEXT("SfxVolume_Description", "Adjusts the volume of sound effects."));
		
		Setting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetSfxVolume));
		Setting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetSfxVolume));
		Setting->SetDefaultValue(1.0f);
		Setting->SetDisplayFormat(UGameSettingValueScalarDynamic::ZeroToOnePercent);
		Setting->SetSourceRangeAndStep(TRange<double>(0.0,1.0), 0.01);
		Screen->AddSetting(Setting);
	}

	// Music volume
	{
		UGameSettingValueScalarDynamic* Setting = NewObject<UGameSettingValueScalarDynamic>();
		Setting->SetDevName(TEXT("MusicVolume"));
		Setting->SetDisplayName(LOCTEXT("MusicVolume_Name","Music Volume"));
		Setting->SetDescriptionRichText(LOCTEXT("MusicVolume_Description", "Adjusts the volume of music."));
		
		Setting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetMusicVolume));
		Setting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetMusicVolume));
		Setting->SetDefaultValue(1.0f);
		Setting->SetDisplayFormat(UGameSettingValueScalarDynamic::ZeroToOnePercent);
		Setting->SetSourceRangeAndStep(TRange<double>(0.0,1.0), 0.01);
		Screen->AddSetting(Setting);
	}

	// UI volume
	{
		UGameSettingValueScalarDynamic* Setting = NewObject<UGameSettingValueScalarDynamic>();
		Setting->SetDevName(TEXT("UIVolume"));
		Setting->SetDisplayName(LOCTEXT("UIVolume_Name","UI Volume"));
		Setting->SetDescriptionRichText(LOCTEXT("UIVolume_Description", "Adjusts the volume of UI sounds."));
		
		Setting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetUIVolume));
		Setting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetUIVolume));
		Setting->SetDefaultValue(1.0f);
		Setting->SetDisplayFormat(UGameSettingValueScalarDynamic::ZeroToOnePercent);
		Setting->SetSourceRangeAndStep(TRange<double>(0.0,1.0), 0.01);
		Screen->AddSetting(Setting);
	}

	return Screen;
}

UGameSettingCollection* UASGameSettingRegistry::InitializeVideoSettings(UASLocalPlayer* InLocalPlayer)
{
	UGameSettingCollection* Screen = NewObject<UGameSettingCollection>();
	Screen->SetDevName(TEXT("VideoCollection"));
	Screen->SetDisplayName(LOCTEXT("VideoCollection_Name","Video"));
	Screen->Initialize(InLocalPlayer);
	
	UGameSettingValueDiscreteDynamic_Enum* WindowModeSetting = nullptr;

	// Window mode
	{
		UGameSettingValueDiscreteDynamic_Enum* Setting = NewObject<UGameSettingValueDiscreteDynamic_Enum>();
		Setting->SetDevName(TEXT("WindowMode"));
		Setting->SetDisplayName(LOCTEXT("WindowMode_Name","Window Mode"));
		Setting->SetDescriptionRichText(LOCTEXT("WindowMode_Description", "In Windowed mode you can interact with other windows more easily, and drag the edges of the window to set the size. In Windowed Fullscreen mode you can easily switch between applications. In Fullscreen mode you cannot interact with other windows as easily, but the game will run slightly faster."));
		
		Setting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetWindowModeEnum));
		Setting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetWindowModeEnum));
		Setting->AddEnumOption(EASWindowMode::Fullscreen,    LOCTEXT("WM_Full",  "Fullscreen"));
		Setting->AddEnumOption(EASWindowMode::Borderless, LOCTEXT("WM_Bord",  "Borderless"));
		Setting->AddEnumOption(EASWindowMode::Windowed,   LOCTEXT("WM_Wind", "Windowed"));
		Setting->SetDefaultValue(EASWindowMode::Borderless);
		Screen->AddSetting(Setting);
		WindowModeSetting = Setting;
	}

	// Resolution
	{
		UASSettingValueDiscrete_Resolution* Setting = NewObject<UASSettingValueDiscrete_Resolution>();
		Setting->SetDevName(TEXT("Resolution"));
		Setting->SetDisplayName(LOCTEXT("Resolution_Name","Resolution"));
		Setting->SetDescriptionRichText(LOCTEXT("Resolution_Description", "Display Resolution determines the size of the window in Windowed mode. In Fullscreen mode, Display Resolution determines the graphics card output resolution, which can result in black bars depending on monitor and graphics card. Display Resolution is inactive in Windowed Fullscreen mode."));

		Setting->AddEditCondition(MakeShared<FASWhenCondition>(
			[](const ULocalPlayer*, FGameSettingEditableState& EditState)
			{
				const UGameUserSettings* GameSettings = UGameUserSettings::GetGameUserSettings();
				if (GameSettings->GetFullscreenMode() == EWindowMode::WindowedFullscreen)
				{
					EditState.Disable(LOCTEXT("Resolution Borderless",
						"Resolution is fixed to the desktop resolution in Borderless mode."));
				}
			}));
		
		Setting->AddEditDependency(WindowModeSetting);
		Screen->AddSetting(Setting);
	}

	// Graphics quality
	{
		UASSettingValueDiscrete_OverallQuality* Setting = NewObject<UASSettingValueDiscrete_OverallQuality>();
		Setting->SetDevName(TEXT("GraphicsPreset"));
		Setting->SetDisplayName(LOCTEXT("GraphicsPreset_Name","Graphics Preset"));
		Setting->SetDescriptionRichText(LOCTEXT("GraphicsPreset_Description", "Quality Preset allows you to adjust multiple video options at once. Try a few options to see what fits your preference and device's performance."));
		Screen->AddSetting(Setting);

		AddQualityBucket(Screen, Setting, TEXT("ViewDistanceQuality"), LOCTEXT("VD_Name","View Distance"),
		LOCTEXT("ViewDistanceQuality_Description", "How far objects render."),
		GET_LOCAL_SETTINGS_FUNCTION_PATH(GetViewDistanceQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetViewDistanceQuality));
		
		AddQualityBucket(Screen, Setting, TEXT("ShadowQuality"), LOCTEXT("Sh_Name","Shadows"),
		LOCTEXT("ShadowQuality_Description", "Shadow detail and distance."),
		GET_LOCAL_SETTINGS_FUNCTION_PATH(GetShadowQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetShadowQuality));
		
		AddQualityBucket(Screen, Setting, TEXT("AntiAliasingQuality"), LOCTEXT("AA_Name","Anti-Aliasing"),
		LOCTEXT("AntiAliasingQuality_Description", "Smooths jagged edges."),
		GET_LOCAL_SETTINGS_FUNCTION_PATH(GetAntiAliasingQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetAntiAliasingQuality));
		
		AddQualityBucket(Screen, Setting, TEXT("TextureQuality"), LOCTEXT("TextureQuality_Name","Textures"),
		LOCTEXT("TextureQuality_Description", "Texture quality determines the resolution of textures in game. Increasing this setting will make objects more detailed, but can reduce performance."),
		GET_LOCAL_SETTINGS_FUNCTION_PATH(GetTextureQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetTextureQuality));
		
		AddQualityBucket(Screen, Setting, TEXT("VisualEffectQuality"), LOCTEXT("Fx_Name","Effects"),
		LOCTEXT("VisualEffectQuality_Description", "Quality of visual effects and lighting."),
		GET_LOCAL_SETTINGS_FUNCTION_PATH(GetVisualEffectQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetVisualEffectQuality));
		
		AddQualityBucket(Screen, Setting, TEXT("PostProcessingQuality"), LOCTEXT("PP_Name","Post Processing"),
		LOCTEXT("PostProcessingQuality_Description", "Bloom, motion blur, depth of field."),
		GET_LOCAL_SETTINGS_FUNCTION_PATH(GetPostProcessingQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetPostProcessingQuality));
	}
	
	// FrameCap
	{
		UGameSettingValueDiscreteDynamic_Number* Setting = NewObject<UGameSettingValueDiscreteDynamic_Number>();
		Setting->SetDevName(TEXT("FrameCap"));
		Setting->SetDisplayName(LOCTEXT("FrameCap_Name","Frame Cap"));
		Setting->SetDescriptionRichText(LOCTEXT("FrameCap_Description", "Limit amount of frames in game. May increase performance."));

		Setting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetFrameRateLimit));
		Setting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetFrameRateLimit));
		Setting->AddOption(30.f,  LOCTEXT("FC_30","30"));
		Setting->AddOption(60.f,  LOCTEXT("FC_60","60"));
		Setting->AddOption(120.f, LOCTEXT("FC_120","120"));
		Setting->AddOption(144.f, LOCTEXT("FC_144","144"));
		Setting->AddOption(0.f,   LOCTEXT("FC_Unlimited","Unlimited"));
		Setting->SetDefaultValue(0.f);
		Screen->AddSetting(Setting);
	}
	
	//Vsync (bool)
	{
		UGameSettingValueDiscreteDynamic_Bool* Setting = NewObject<UGameSettingValueDiscreteDynamic_Bool>();
		Setting->SetDevName(TEXT("VSync"));
		Setting->SetDisplayName(LOCTEXT("VSync_Name","VSync"));
		Setting->SetDescriptionRichText(LOCTEXT("VSync_Description", "Enabling Vertical Sync eliminates screen tearing by always rendering and presenting a full frame. Disabling Vertical Sync can give higher frame rate and better input response, but can result in horizontal screen tearing."));
		
		Setting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetVSync));
		Setting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetVSync));
		Setting->SetDefaultValue(false);
		Screen->AddSetting(Setting);
	}
	
	// TODO every setting from graphics quality
	
	return Screen;
}

UGameSettingCollection* UASGameSettingRegistry::InitializeControlSettings(UASLocalPlayer* InLocalPlayer)
{
	UGameSettingCollection* Screen = NewObject<UGameSettingCollection>();
	Screen->SetDevName(TEXT("ControlsCollection"));
	Screen->SetDisplayName(LOCTEXT("ControlsCollection_Name", "Controls"));
	Screen->Initialize(InLocalPlayer);

	{
		UGameSettingValueScalarDynamic* Setting = NewObject<UGameSettingValueScalarDynamic>();
		Setting->SetDevName(TEXT("LookSensitivity"));
		Setting->SetDisplayName(LOCTEXT("LookSensitivity_Name","Look Sensitivity"));
		Setting->SetDescriptionRichText(LOCTEXT("LookSensitivity_Description","How fast the camera turns with mouse movement."));
		
		Setting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetLookSensitivity));
		Setting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetLookSensitivity));
		Setting->SetDefaultValue(1.0f);
		Setting->SetDisplayFormat(UGameSettingValueScalarDynamic::RawTwoDecimals);
		Setting->SetSourceRangeAndStep(TRange<double>(0.0,3.0), 0.01);
		Screen->AddSetting(Setting);
	}
	
	return Screen;
}

void UASGameSettingRegistry::SaveChanges()
{
	Super::SaveChanges();
	if (UASGameUserSettings* Settings = UASGameUserSettings::GetASGameUserSettings())
	{
		Settings->ApplySettings(false);
	}
}

#undef LOCTEXT_NAMESPACE
#undef GET_LOCAL_SETTINGS_FUNCTION_PATH
