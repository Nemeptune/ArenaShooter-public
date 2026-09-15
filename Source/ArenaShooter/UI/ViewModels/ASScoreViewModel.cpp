// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModels/ASScoreViewModel.h"

void UASScoreViewModel::SetLocalKills(int32 Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(LocalKills, Value);
}

void UASScoreViewModel::SetOpponentKills(int32 Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(OpponentKills, Value);
}

void UASScoreViewModel::SetLocalName(const FText& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(LocalName, Value);
}

void UASScoreViewModel::SetOpponentName(const FText& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(OpponentName, Value);
}

void UASScoreViewModel::SetMatchTimeText(const FText& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(MatchTimeText, Value);
}

void UASScoreViewModel::SetLocalAvatar(UTexture2D* Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(LocalAvatar, Value);
}

void UASScoreViewModel::SetOpponentAvatar(UTexture2D* Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(OpponentAvatar, Value);
}
