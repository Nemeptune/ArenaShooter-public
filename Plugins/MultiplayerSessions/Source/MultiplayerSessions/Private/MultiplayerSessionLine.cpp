// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionLine.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSubsystem.h"

void UMultiplayerSessionLine::SetupSessionLineText(FOnlineSessionSearchResult Result, FString InAndMaxPlayers)
{
	SessionResult = Result;
	FString SessionName;
	Result.Session.SessionSettings.Get(FName("SESSION_NAME"), SessionName);
	SessionNameText->SetText(FText::FromString(SessionName));
	PlayersInSessionText->SetText(FText::FromString(InAndMaxPlayers));
}

bool UMultiplayerSessionLine::Initialize()
{
	if (!Super::Initialize()) { return false; }
	if (JoinButton)
	{
		JoinButton->OnClicked.AddUniqueDynamic(this, &ThisClass::JoinButtonOnClicked);
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
	}
	return true;
}

void UMultiplayerSessionLine::OnJoinSession(FName SessionName,EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::UnknownError)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Yellow,
			FString::Printf(TEXT("UnknownError"))
		);
		return;
	}
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid()) 
		{
			FString Adress;
			SessionInterface->GetResolvedConnectString(SessionName, Adress);

			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						-1,
						15.f,
						FColor::Yellow,
						FString::Printf(TEXT("trying to client travel"))
					);
				}
				PlayerController->ClientTravel(Adress, ETravelType::TRAVEL_Absolute);
			}
		}
	}
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		JoinButton->SetIsEnabled(true);
	}

}

void UMultiplayerSessionLine::JoinButtonOnClicked()
{
	JoinButton->SetIsEnabled(false);
	if (!MultiplayerSessionsSubsystem) { return; }
	FString SessionName;
	SessionResult.Session.SessionSettings.Get(FName("SESSION_NAME"), SessionName);
	MultiplayerSessionsSubsystem->JoinSession(SessionResult, FName(*SessionName));
}
