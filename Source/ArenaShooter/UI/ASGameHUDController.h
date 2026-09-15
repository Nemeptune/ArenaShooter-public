// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ASGameHUDController.generated.h"

class UASLocalPlayer;
class UASResultWidget;
class AASPlayerController;
class AASPlayerState;
class UASVitalsViewModel;
class UASScoreViewModel;
class UASVitalsBinder;
class UASScoreBinder;
class UASHotbarBinder;
class UASHUDWidget;
class UCommonActivatableWidget;
class UASUILayerManager;
struct FASMatchEndedMessage;

UCLASS()
class ARENASHOOTER_API UASGameHUDController : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	void OpenGameMenu();
	
private:
	void HandlePlayerControllerSet(UASLocalPlayer* LocalPlayer, APlayerController* PC);
	void HandlePlayerStateSet(UASLocalPlayer* LocalPlayer, APlayerState* PS);
	bool EnsureHUD();
	void EnsureBindings(AASPlayerController* PC, AASPlayerState* PS);
	void AssignViewModel();
	void ResetState();
	
	void EnsureMatchListener();
	void HandleMatchEndedMessage(FGameplayTag Channel, const FASMatchEndedMessage& Message);
	void ShowResultScreen(APlayerState* Winner, bool bDraw);

	UASUILayerManager* GetLayerManager();
	
	TWeakObjectPtr<AASPlayerController> BoundPC;
	
	UPROPERTY()
	TObjectPtr<UASVitalsViewModel> VitalsVM;
	UPROPERTY()
	TObjectPtr<UASVitalsBinder> VitalsBinder;
	UPROPERTY()
	TObjectPtr<UASScoreViewModel> ScoreVM;
	UPROPERTY()
	TObjectPtr<UASScoreBinder> ScoreBinder;
	UPROPERTY()
	TObjectPtr<UASHotbarBinder> HotbarBinder;
	UPROPERTY()
	TObjectPtr<UASHUDWidget> HUDWidget;
	UPROPERTY()
	TObjectPtr<AASPlayerState> LocalPS;
	
	FGameplayMessageListenerHandle MatchEndedHandle;
	bool bResultShown = false;
};
