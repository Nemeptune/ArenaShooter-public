// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ASUILayerManager.h"

#include "Player/ASLocalPlayer.h"
#include "System/ASLogChannels.h"
#include "ASRootLayout.h"
#include "ASUIConfig.h"
#include "ASUISettings.h"
#include "CommonActivatableWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UASUILayerManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (UASLocalPlayer* LP = Cast<UASLocalPlayer>(GetLocalPlayer()))
	{
		LP->CallAndRegister_OnPlayerControllerSet(UASLocalPlayer::FASPlayerControllerSet::FDelegate::CreateUObject(this, &UASUILayerManager::HandlePlayerControllerSet));
	}
}

void UASUILayerManager::HandlePlayerControllerSet(UASLocalPlayer* LocalPlayer, APlayerController* PC)
{
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	
	if (RootLayout)
	{
		// Same object, new world. Layers stay registered because the layout is never destroyed
		// SetPlayerContext re-points it at the new world before re-parenting.
		RootLayout->RemoveFromParent();
		RootLayout->SetPlayerContext(FLocalPlayerContext(LocalPlayer, PC->GetWorld()));
		RootLayout->AddToViewport();
		return;
	}
	
	UASUIConfig* UIConfig = LocalPlayer->GetUIConfig();
	TSubclassOf<UASRootLayout> LayoutClass = UIConfig ? UIConfig->RootLayoutClass.LoadSynchronous() : nullptr;
	if (!LayoutClass)
	{
		UE_LOG(LogAS, Warning, TEXT("No RootLayoutClass (Project Settings > UIConfig)."));
		return;
	}
	
	RootLayout = CreateWidget<UASRootLayout>(PC, LayoutClass);
	if (RootLayout)
	{
		RootLayout->AddToViewport();
	}
}

void UASUILayerManager::PopWidgetFromLayer(UCommonActivatableWidget* Widget)
{
	if (!Widget)
	{
		return;
	}
	
	for (const TPair<FGameplayTag, TWeakObjectPtr<UCommonActivatableWidgetStack>>& Pair : LayerMap)
	{
		if (UCommonActivatableWidgetStack* Stack = Pair.Value.Get())
		{
			Stack->RemoveWidget(*Widget);
		}
	}
}

void UASUILayerManager::RegisterLayer(FGameplayTag LayerTag, UCommonActivatableWidgetStack* Stack)
{
	if (LayerTag.IsValid() && Stack)
	{
		LayerMap.Add(LayerTag, Stack);
	}
}

UCommonActivatableWidget* UASUILayerManager::PushWidgetToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!WidgetClass)
	{
		return nullptr;
	}
	if (UCommonActivatableWidgetStack* Stack = GetLayerStack(LayerTag))
	{
		return Stack->AddWidget<UCommonActivatableWidget>(WidgetClass);
	}
	
	return nullptr;
}

void UASUILayerManager::PushWidgetToLayerAsync(FGameplayTag LayerTag,
	TSoftClassPtr<UCommonActivatableWidget> WidgetClass, FOnWidgetPushed OnPushed)
{
	if (WidgetClass.IsNull())
	{
		OnPushed.ExecuteIfBound(nullptr);
		return;
	}

	// Skip async hop if already in memory
	if (WidgetClass.Get())
	{
		OnPushed.ExecuteIfBound(PushWidgetToLayer(LayerTag, WidgetClass.Get()));
		return;
	}

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	Streamable.RequestAsyncLoad(
		WidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &UASUILayerManager::HandleWidgetLoaded, LayerTag, WidgetClass, OnPushed)
	);
}

UCommonActivatableWidgetStack* UASUILayerManager::GetLayerStack(FGameplayTag LayerTag) const
{
	const TWeakObjectPtr<UCommonActivatableWidgetStack>* Found = LayerMap.Find(LayerTag);
	return (Found && Found->IsValid()) ? Found->Get() : nullptr;
}

void UASUILayerManager::HandleWidgetLoaded(FGameplayTag LayerTag, TSoftClassPtr<UCommonActivatableWidget> WidgetClass, FOnWidgetPushed OnPushed)
{
	UClass* Loaded = WidgetClass.Get();

	// if the player travelled while loading, the stack's weakptr is now stale -> PushWidgetToLayer returns null safely
	OnPushed.ExecuteIfBound(Loaded ? PushWidgetToLayer(LayerTag, WidgetClass.Get()) : nullptr);
}
