// Fill out your copyright notice in the Description page of Project Settings.


#include "ASMenuPlayerController.h"

#include "CommonActivatableWidget.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ASUILayerManager.h"
#include "UI/ASUITags.h"

void AASMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), FoundCameras);

	if (FoundCameras.Num() > 0)
	{
		AActor* MenuCamera = FoundCameras[0];
		SetViewTarget(MenuCamera);
	}

	if (!IsLocalController())
	{
		return;
	}

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UASUILayerManager* LM = LP->GetSubsystem<UASUILayerManager>())
		{
			if (MenuClass)
			{
				MenuWidget = LM->PushWidgetToLayer(ASUITags::Layer_Menu, MenuClass);
			}
		}
	}
}

void AASMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MenuWidget)
	{
		if (ULocalPlayer* LP = GetLocalPlayer())
		{
			if (UASUILayerManager* LM = LP->GetSubsystem<UASUILayerManager>())
			{
				LM->PopWidgetFromLayer(MenuWidget);
			}
		}
		MenuWidget = nullptr;
	}
	
	Super::EndPlay(EndPlayReason);
}
