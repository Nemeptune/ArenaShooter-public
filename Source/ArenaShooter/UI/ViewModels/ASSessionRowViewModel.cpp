// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModels/ASSessionRowViewModel.h"
#include "UI/ViewModels/ASJoinViewModel.h"

void UASSessionRowViewModel::Setup(UASJoinViewModel* InOwner, const FOnlineSessionSearchResult& InResult, const FText& InName, const FText& InMap, const FText& InPlayers)
{
	Owner = InOwner;
	Result = InResult;
	SetSessionName(InName);
	SetMapName(InMap);
	SetPlayerText(InPlayers);
	SetPingText(FText::FromString(FString::Printf(TEXT("%d ms"), InResult.PingInMs)));
}

void UASSessionRowViewModel::Join()
{
	if (Owner.IsValid())
	{
		Owner->JoinResult(Result);
	}
}

void UASSessionRowViewModel::SetSessionName(const FText& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(SessionName, Value);
}

void UASSessionRowViewModel::SetMapName(const FText& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(MapName, Value);
}

void UASSessionRowViewModel::SetPlayerText(const FText& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(PlayersText, Value);
}

void UASSessionRowViewModel::SetPingText(const FText& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(PingText, Value);
}
