// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModels/ASJoinViewModel.h"
#include "UI/ViewModels/ASSessionRowViewModel.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "OnlineSessionSettings.h"

void UASJoinViewModel::Initialize(APlayerController* OwningPC)
{
	if (!OwningPC)
	{
		return;
	}
	if (UGameInstance* GI = OwningPC->GetGameInstance())
	{
		SessionsSubsystem = GI->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (SessionsSubsystem.IsValid())
		{
			SessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &UASJoinViewModel::HandleFindComplete);
			SessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &UASJoinViewModel::HandleJoinComplete);
		}
	}
}

void UASJoinViewModel::RefreshSessions()
{
	if (!SessionsSubsystem.IsValid() || bIsBusy)
	{
		return;
	}
	SetIsBusy(true);
	SetStatusText(FText::FromString(TEXT("Searching...")));
	SetSessions({});
	SessionsSubsystem->FindSessions(MaxSearchResults);
}

void UASJoinViewModel::JoinResult(const FOnlineSessionSearchResult& Result)
{
	if (!SessionsSubsystem.IsValid() || bIsBusy)
	{
		return;
	}
	SetIsBusy(true);
	SetStatusText(FText::FromString(TEXT("Joining...")));
	FString Name;
	Result.Session.SessionSettings.Get(FName("SESSION_NAME"), Name);
	SessionsSubsystem->JoinSession(Result, FName(*Name));
}

void UASJoinViewModel::HandleFindComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful)
{
	SetIsBusy(false);

	TArray<TObjectPtr<UASSessionRowViewModel>> NewRows;
	for (const FOnlineSessionSearchResult& Result : Results)
	{
		FString FoundMatchType;
		Result.Session.SessionSettings.Get(FName("MatchType"), FoundMatchType);
		if (FoundMatchType != MatchType)
		{
			continue;
		}

		FString Name, Map;
		Result.Session.SessionSettings.Get(FName("SESSION_NAME"), Name);
		Result.Session.SessionSettings.Get(FName("SESSION_MAP"), Map);

		const int32 MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		const int32 CurPlayers = MaxPlayers - Result.Session.NumOpenPublicConnections;

		UASSessionRowViewModel* Row = NewObject<UASSessionRowViewModel>(this);
		Row->Setup(this, Result, FText::FromString(Name), FText::FromString(Map), FText::FromString(FString::Printf(TEXT("%d/%d"), CurPlayers, MaxPlayers)));
		NewRows.Add(Row);
	}

	SetSessions(NewRows);
	UE_LOG(LogTemp, Warning, TEXT("Sessions are %s"), NewRows.Num() > 0 ? TEXT("Found") : TEXT("Not Found"));
	SetStatusText(FText::FromString(TEXT("Searching complete")));
}

void UASJoinViewModel::HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		SetIsBusy(false);
		SetStatusText(FText::FromString(TEXT("Failed to join session")));
	}
}

void UASJoinViewModel::SetSessions(const TArray<TObjectPtr<UASSessionRowViewModel>>& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(Sessions, Value);
}

void UASJoinViewModel::SetStatusText(const FText& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(StatusText, Value);
}

void UASJoinViewModel::SetIsBusy(bool Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsBusy, Value);
}

void UASJoinViewModel::BeginDestroy()
{
	if (SessionsSubsystem.IsValid())
	{
		SessionsSubsystem->MultiplayerOnFindSessionsComplete.RemoveAll(this);
		SessionsSubsystem->MultiplayerOnJoinSessionComplete.RemoveAll(this);
	}
	Super::BeginDestroy();
}
