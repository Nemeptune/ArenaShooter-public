// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModels/ASHostViewModel.h"
#include "MultiplayerSessionsSubsystem.h"

void UASHostViewModel::Initialize(APlayerController* OwningPC)
{
	if (!OwningPC)
	{
		return;
	}

	if (UGameInstance* GI = OwningPC->GetGameInstance())
	{
		Sessions = GI->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (Sessions.IsValid())
		{
			Sessions->MultiplayerOnCreateSessionComplete.AddDynamic(this, &UASHostViewModel::HandleCreateComplete);
		}
	}
}

void UASHostViewModel::HostGame(const FText& SessionName)
{
	if (!Sessions.IsValid() || bIsBusy)
	{
		return;
	}

	SetIsBusy(true);
	SetStatusText(FText::FromString(TEXT("Creating session...")));
	Sessions->CreateSession(NumConnections, MatchType, FName(*SessionName.ToString()), MapOptions[SelectedMapIndex].Map);
}

void UASHostViewModel::InitMaps()
{
	RebuildMapLabels();
	SetSelectedMapIndex(0);
}

void UASHostViewModel::InitMapsFrom(const TArray<FASMapOption>& InMaps)
{
	MapOptions = InMaps;
	InitMaps();
}

void UASHostViewModel::RebuildMapLabels()
{
	MapLabels.Reset();
	for (const FASMapOption& Option : MapOptions)
	{
		MapLabels.Add(Option.DisplayName);
	}
}

void UASHostViewModel::SetSelectedMapIndex(int32 Index)
{
	const int32 Clamped = MapOptions.Num() > 0 ? FMath::Clamp(Index, 0, MapOptions.Num() - 1) : 0;
	UE_MVVM_SET_PROPERTY_VALUE(SelectedMapIndex, Clamped);
	UpdatePreview();
}

void UASHostViewModel::SetPreviewTexture(UTexture2D* Texture)
{
	UE_MVVM_SET_PROPERTY_VALUE(PreviewTexture, Texture);
}

const TArray<FText>& UASHostViewModel::GetMapLabels() const
{
	return MapLabels;
}

void UASHostViewModel::UpdatePreview()
{
	UTexture2D* Tex = MapOptions.IsValidIndex(SelectedMapIndex) ? MapOptions[SelectedMapIndex].Preview : nullptr;
	SetPreviewTexture(Tex);
}

void UASHostViewModel::HandleCreateComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		SetStatusText(FText::FromString(TEXT("Session created, starting...")));
	}
	else
	{
		SetIsBusy(false);
		SetStatusText(FText::FromString(TEXT("Failed to create session")));
	}
}

void UASHostViewModel::SetStatusText(const FText& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(StatusText, Value);
}

void UASHostViewModel::SetIsBusy(bool Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsBusy, Value);
}

void UASHostViewModel::BeginDestroy()
{
	if (Sessions.IsValid())
	{
		Sessions->MultiplayerOnCreateSessionComplete.RemoveDynamic(this, &UASHostViewModel::HandleCreateComplete);
	}
	Super::BeginDestroy();
}
