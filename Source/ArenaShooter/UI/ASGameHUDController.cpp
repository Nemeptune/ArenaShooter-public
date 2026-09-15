// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ASGameHUDController.h"

#include "GameModes/ASGameState.h"
#include "Binders/ASHotbarBinder.h"
#include "ASHUDWidget.h"
#include "System/ASLogChannels.h"
#include "Player/ASPlayerState.h"
#include "ASResultWidget.h"
#include "Binders/ASScoreBinder.h"
#include "ASUILayerManager.h"
#include "ASUITags.h"
#include "ViewModels/ASVitalsViewModel.h"
#include "ViewModels/ASScoreViewModel.h"
#include "ASUIConfig.h"
#include "Binders/ASVitalsBinder.h"
#include "MVVMSubsystem.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "View/MVVMView.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Messages/ASMessageTags.h"
#include "Messages/ASUIMessages.h"
#include "Player/ASLocalPlayer.h"
#include "Player/ASPlayerController.h"

// TODO too much silent returns in this class, if smth goes wrong there are no clue what caused it

void UASGameHUDController::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency(UASUILayerManager::StaticClass());
	Super::Initialize(Collection);
	
	if (UASLocalPlayer* LP = Cast<UASLocalPlayer>(GetLocalPlayer()))
	{
		LP->CallAndRegister_OnPlayerControllerSet(UASLocalPlayer::FASPlayerControllerSet::FDelegate::CreateUObject(this, &UASGameHUDController::HandlePlayerControllerSet));
		LP->CallAndRegister_OnPlayerStateSet(UASLocalPlayer::FASPlayerStateSet::FDelegate::CreateUObject(this, &UASGameHUDController::HandlePlayerStateSet));
	}
}

void UASGameHUDController::HandlePlayerControllerSet(UASLocalPlayer* LocalPlayer, APlayerController* PC)
{
	ResetState();
	
	BoundPC = Cast<AASPlayerController>(PC);
}

void UASGameHUDController::HandlePlayerStateSet(UASLocalPlayer* LocalPlayer, APlayerState* PS)
{
	AASPlayerController* PlayerController = BoundPC.Get();
	AASPlayerState* PlayerState = Cast<AASPlayerState>(PS);
	if (!PlayerController || !PlayerState)
	{
		return;
	}
	
	LocalPS = PlayerState;
	
	if (!EnsureHUD())
	{
		return;
	}
	EnsureMatchListener();
	EnsureBindings(PlayerController, PlayerState);
}

void UASGameHUDController::Deinitialize()
{
	ResetState();
	Super::Deinitialize();
}

bool UASGameHUDController::EnsureHUD()
{
	if (HUDWidget)
	{
		return true;
	}

	UASLocalPlayer* LP = Cast<UASLocalPlayer>(GetLocalPlayer());
	UASUIConfig* UIConfig = LP ? LP->GetUIConfig() : nullptr;
	TSubclassOf<UASHUDWidget> HUDClass = UIConfig ? UIConfig->HUDWidgetClass.LoadSynchronous() : nullptr;
	if (!HUDClass)
	{
		UE_LOG(LogAS, Warning, TEXT("EnsureHUD: no HUDWidgetClass (Project Settings > UIConfig)."));
		return false;
	}
	HUDWidget = Cast<UASHUDWidget>(GetLayerManager()->PushWidgetToLayer(ASUITags::Layer_Game, HUDClass));
	if (!HUDWidget)
	{
		return false;
	}
	
	VitalsVM = NewObject<UASVitalsViewModel>(this);
	ScoreVM = NewObject<UASScoreViewModel>(this);

	AssignViewModel();
	return true;
}

void UASGameHUDController::EnsureBindings(AASPlayerController* PC, AASPlayerState* PS)
{
	if (!ScoreBinder)
	{
		ScoreBinder = NewObject<UASScoreBinder>(this);
		ScoreBinder->Init(LocalPS,ScoreVM);
	}
	if (!HotbarBinder)
	{
		HotbarBinder = NewObject<UASHotbarBinder>(this);
		HotbarBinder->Init(PC, HUDWidget);
	}
	
	UASAbilitySystemComponent* ASC = PS->GetASAbilityComponent();
	if (!ASC)
	{
		return;
	}
	if (!VitalsBinder)
	{
		VitalsBinder = NewObject<UASVitalsBinder>(this);
		VitalsBinder->Init(ASC, VitalsVM);
	}
}

void UASGameHUDController::AssignViewModel()
{
	if (!HUDWidget || !VitalsVM || !ScoreVM) return;
	if (UMVVMSubsystem* Sub = GEngine->GetEngineSubsystem<UMVVMSubsystem>())
	{
		if (UMVVMView* View = Sub->GetViewFromUserWidget(HUDWidget))
		{
			View->SetViewModelByClass(VitalsVM);
			View->SetViewModelByClass(ScoreVM);
		}
	}
}

void UASGameHUDController::OpenGameMenu()
{
	UASUILayerManager* Layers = GetLayerManager();
	if (!Layers)
	{
		return;
	}

	if (UCommonActivatableWidgetStack* Stack = Layers->GetLayerStack(ASUITags::Layer_GameMenu))
	{
		if (Stack->GetActiveWidget())
		{
			return; // prevent double open and opening over the end of the match screen
		}
	}

	UASLocalPlayer* LP = Cast<UASLocalPlayer>(GetLocalPlayer());
	UASUIConfig* UIConfig = LP ? LP->GetUIConfig() : nullptr;
	if (TSubclassOf<UCommonActivatableWidget> Widget = UIConfig ? UIConfig->GameMenuClass.LoadSynchronous() : nullptr)
	{
		Layers->PushWidgetToLayer(ASUITags::Layer_GameMenu, Widget);
	}
}

UASUILayerManager* UASGameHUDController::GetLayerManager()
{
	return GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UASUILayerManager>() : nullptr;
}

void UASGameHUDController::ResetState()
{
	if (HotbarBinder)
	{
		HotbarBinder->Shutdown();
	}
	if (VitalsBinder)
	{
		VitalsBinder->Shutdown();
	}
	if (ScoreBinder)
	{
		ScoreBinder->Shutdown();
	}
	HotbarBinder = nullptr;
	ScoreBinder = nullptr;
	VitalsBinder = nullptr;
	
	if (HUDWidget)
	{
		if (UASUILayerManager* Layers = GetLayerManager())
		{
			Layers->PopWidgetFromLayer(HUDWidget);
		}
	}
	if (MatchEndedHandle.IsValid())
	{
		MatchEndedHandle.Unregister();
	}
	bResultShown = false;
	
	HUDWidget = nullptr;
	VitalsVM = nullptr;
	ScoreVM = nullptr;
	LocalPS = nullptr;
}

void UASGameHUDController::EnsureMatchListener()
{
	if (MatchEndedHandle.IsValid())
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	MatchEndedHandle = UGameplayMessageSubsystem::Get(World).RegisterListener(FASMessageTags::Match_Ended, this, &UASGameHUDController::HandleMatchEndedMessage);
	
	if (AASGameState* GS = World->GetGameState<AASGameState>())
	{
		if (GS->HasMatchEnded())
		{
			ShowResultScreen(GS->GetWinner(), GS->GetWinner() == nullptr);
		}
	}
}

void UASGameHUDController::HandleMatchEndedMessage(FGameplayTag Channel, const FASMatchEndedMessage& Message)
{
	ShowResultScreen(Message.Winner, Message.bDraw);
}

void UASGameHUDController::ShowResultScreen(APlayerState* Winner, bool bDraw)
{
	if (bResultShown)
	{
		return;
	}
	
	UASLocalPlayer* LocalPlayer = Cast<UASLocalPlayer>(GetLocalPlayer());
	UASUIConfig* UIConfig = LocalPlayer ? LocalPlayer->GetUIConfig() : nullptr;
	TSubclassOf<UASCommonActivatableWidget> ResultClass = UIConfig ? UIConfig->ResultWidgetClass.LoadSynchronous() : nullptr;
	if (!ResultClass)
	{
		UE_LOG(LogAS, Warning, TEXT("ShowResultScreen: no ResultWidgetClass in UIConfig."));
		return;
	}
	
	const bool bWon = !bDraw && (Winner == LocalPS);
	
	if (UASUILayerManager* Layers = GetLayerManager())
	{
		if (UCommonActivatableWidget* Widget = Layers->PushWidgetToLayer(ASUITags::Layer_GameMenu, ResultClass))
		{
			Cast<UASResultWidget>(Widget)->InitResult(bWon, bDraw);
			bResultShown = true;
		}
	}
}
