// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ASRootLayout.h"
#include "ASUILayerManager.h"
#include "ASUITags.h"


void UASRootLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UASUILayerManager* LM = LP->GetSubsystem<UASUILayerManager>())
		{
			LM->RegisterLayer(ASUITags::Layer_Game, GameStack);
			LM->RegisterLayer(ASUITags::Layer_GameMenu, GameMenuStack);
			LM->RegisterLayer(ASUITags::Layer_Menu, MenuStack);
			LM->RegisterLayer(ASUITags::Layer_Modal, ModalStack);
		}
	}
}
