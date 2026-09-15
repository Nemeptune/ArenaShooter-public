// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ASResultWidget.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UASResultWidget::InitResult(bool bWon, bool bDraw)
{
	OnResultSet(bWon, bDraw);
}

TOptional<FUIInputConfig> UASResultWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UASResultWidget::ReturnToMainMenu()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UMultiplayerSessionsSubsystem* Sessions = GI->GetSubsystem<UMultiplayerSessionsSubsystem>())
		{
			Sessions->LeaveSessionAndTravel(MainMenuLevel);
			return;
		}
	}
	if (!MainMenuLevel.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, MainMenuLevel);
	}
}
