// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ASSettingsScreen.h"
#include "Input/CommonUIInputTypes.h"
#include "Player/ASLocalPlayer.h"
#include "Settings/ASGameSettingRegistry.h"


void UASSettingsScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//BackHandle = RegisterUIActionBinding(FBindUIActionArgs(BackInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleBackAction)));
}

void UASSettingsScreen::NativeOnDeactivated()
{
	if (HaveSettingsBeenChanged())
	{
		CancelChanges();
	}
	Super::NativeOnDeactivated();
}

UGameSettingRegistry* UASSettingsScreen::CreateRegistry()
{
	UASLocalPlayer* LP = Cast<UASLocalPlayer>(GetOwningLocalPlayer());
	if (!ensureAlwaysMsgf(LP, TEXT("Settings: owning LocalPlayer is not UASLocalPlayer — registry won't initialize. Check LocalPlayerClassName and RESTART the editor.")))
	{
		return nullptr;
	}

	UASGameSettingRegistry* NewRegistry = NewObject<UASGameSettingRegistry>();
	NewRegistry->Initialize(LP);
	return NewRegistry;
}

void UASSettingsScreen::HandleBackAction()
{
	if (AttemptToPopNavigation())
	{
		return;
	}

	DeactivateWidget();
}
