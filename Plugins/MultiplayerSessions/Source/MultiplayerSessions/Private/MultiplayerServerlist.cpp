// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerServerlist.h"
#include "MultiplayerSessionLine.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"

bool UMultiplayerServerlist::Initialize()
{
	if (!Super::Initialize()) {return false; }

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
	}
	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RefreshSessions);
	}
	return true;
}

void UMultiplayerServerlist::RefreshSessions()
{
	SessionList->ClearChildren();
	if (!MultiplayerSessionsSubsystem) { return; }
	MultiplayerSessionsSubsystem->FindSessions(10000);
}

void UMultiplayerServerlist::AddSessionToSessionList(FOnlineSessionSearchResult Result, FString InAndMaxPlayers)
{
	UMultiplayerSessionLine* SessionLine = CreateWidget<UMultiplayerSessionLine>(this, SessionLineClass);
	SessionLine->SetupSessionLineText(Result, InAndMaxPlayers);
	SessionList->AddChild(SessionLine);
}

void UMultiplayerServerlist::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (MultiplayerSessionsSubsystem == nullptr) { return; }
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Cyan,
			FString::Printf(TEXT("FindingSession, SessionResults: %d"), SessionResults.Num())
		);
	}
	for (auto Result : SessionResults)
	{
		FString SettingsValue, SessionPlayers;
		int32 MaxPlayers, CurrentPlayers;
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
		if (SettingsValue == MatchType)
		{
			MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
			CurrentPlayers = MaxPlayers - Result.Session.NumOpenPublicConnections;
			SessionPlayers = FString(FString::FromInt(CurrentPlayers) + "/" + FString::FromInt(MaxPlayers));
			AddSessionToSessionList(Result, SessionPlayers);
			//MultiplayerSessionsSubsystem->JoinSession(Result);
		}
	}
}

