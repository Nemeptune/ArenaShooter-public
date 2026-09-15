// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerMenu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include <Kismet/KismetSystemLibrary.h>

bool UMultiplayerMenu::Initialize()
{
	if (!Super::Initialize()) { return false; }

	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::QuitButtonOnClicked);
	}

	return true;
}

void UMultiplayerMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

void UMultiplayerMenu::OnCreateSession(bool bWasSuccessful)
{
	//if (bWasSuccessful)
	//{
	//	if (GEngine)
	//	{
	//		GEngine->AddOnScreenDebugMessage(
	//			-1,
	//			15.f,
	//			FColor::Yellow,
	//			FString::Printf(TEXT("Session Created successfuly"))
	//		);
	//	}
	//	UWorld* World = GetWorld();
	//	if (World)
	//	{
	//		World->ServerTravel(PathToLobby);
	//	}
	//}else
	//{
	//	if (GEngine)
	//	{
	//		GEngine->AddOnScreenDebugMessage(
	//			-1,
	//			15.f,
	//			FColor::Red,
	//			FString::Printf(TEXT("Session was not Created"))
	//		);
	//	}
	//	//HostButton->SetIsEnabled(true);
	//}
}

//void UMultiplayerMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
//{
//	//Parsing through search results and joining to first correct
//	if (MultiplayerSessionsSubsystem == nullptr) { return; }
//
//	for (auto Result : SessionResults)
//	{
//		FString SettingsValue;
//		FName SessionName;
//		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
//		Result.Session.SessionSettings.Get(FName("SessionName"), SessionName);
//		if (SettingsValue == MatchType)
//		{
//			//Need add every session of our game and add to sessionlist here
//			MultiplayerSessionsSubsystem->JoinSession(Result, SessionName);
//			return;
//		}
//	}
//	if (!bWasSuccessful || SessionResults.Num() <= 0)
//	{
//		JoinButton->SetIsEnabled(true);
//	}
//}

//void UMultiplayerMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
//{
//	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
//	if (OnlineSubsystem)
//	{
//		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
//		if (SessionInterface.IsValid()) 
//		{
//			FString Adress;
//			FName SessionName;
//			SessionInterface->GetResolvedConnectString(SessionName, Adress);
//
//			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
//			if (PlayerController)
//			{
//				PlayerController->ClientTravel(*Adress, ETravelType::TRAVEL_Absolute);
//			}
//		}
//	}
//	if (Result != EOnJoinSessionCompleteResult::Success)
//	{
//		JoinButton->SetIsEnabled(true);
//	}
//
//}

void UMultiplayerMenu::OnDestroySession(FName SessionName,bool bWasSuccessful)
{
}

void UMultiplayerMenu::OnStartSession(bool bWasSuccessful)
{
}

void UMultiplayerMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddUniqueDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddUniqueDynamic(this, &ThisClass::OnStartSession);
	}
}

void UMultiplayerMenu::HostButtonOnClicked()
{
	/*HostButton->SetIsEnabled(false);
	if (!MultiplayerSessionsSubsystem) { return; }
	MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);*/
}

void UMultiplayerMenu::JoinButtonOnClicked()
{
	/*JoinButton->SetIsEnabled(false);
	//if (!MultiplayerSessionsSubsystem) { return; }
	//MultiplayerSessionsSubsystem->FindSessions(10000);*/
}

void UMultiplayerMenu::QuitButtonOnClicked()
{
	UWorld* World = GetWorld();
	UKismetSystemLibrary::QuitGame(World, World->GetFirstPlayerController(),EQuitPreference::Quit,true);
}

void UMultiplayerMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
