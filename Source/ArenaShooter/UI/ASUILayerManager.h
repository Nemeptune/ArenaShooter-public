// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ASUILayerManager.generated.h"

class UASLocalPlayer;
class UCommonActivatableWidget;
class UCommonActivatableWidgetStack;
class UASRootLayout;

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnWidgetPushed, UCommonActivatableWidget*, Widget);

/**
 * Coordinator for the layered UI.
 */
UCLASS()
class ARENASHOOTER_API UASUILayerManager : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	UFUNCTION(BlueprintCallable, Category = "UI|Layers")
	void RegisterLayer(UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag LayerTag, UCommonActivatableWidgetStack* Stack);

	UFUNCTION(BlueprintCallable, Category = "UI|Layers", meta = (DeterminesOutputType = "WidgetClass"))
	UCommonActivatableWidget* PushWidgetToLayer(UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag LayerTag,
												TSubclassOf<UCommonActivatableWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "UI|Layers")
	void PushWidgetToLayerAsync(UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag LayerTag,
								TSoftClassPtr<UCommonActivatableWidget> WidgetClass, FOnWidgetPushed OnPushed);
	
	void PopWidgetFromLayer(UCommonActivatableWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "UI|Layers")
	UCommonActivatableWidgetStack* GetLayerStack(UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag LayerTag) const;
	
	FSimpleMulticastDelegate OnLayoutRebuilt;

private:
	void HandleWidgetLoaded(FGameplayTag LayerTag, TSoftClassPtr<UCommonActivatableWidget> WidgetClass, FOnWidgetPushed OnPushed);
	
	void HandlePlayerControllerSet(UASLocalPlayer* LocalPlayer, APlayerController* PC);
	
	UPROPERTY()
	TMap<FGameplayTag, TWeakObjectPtr<UCommonActivatableWidgetStack>> LayerMap;
	
	UPROPERTY()
	TObjectPtr<UASRootLayout> RootLayout;
};
