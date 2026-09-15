// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerHost.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Kismet/KismetStringLibrary.h" 
#include <Kismet/GameplayStatics.h>

bool UMultiplayerHost::Initialize()
{
	if (!Super::Initialize()) { return false; }
	if (HostSessionButton)
	{
		HostSessionButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HostSessionButtonOnClicked);
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddUniqueDynamic(this, &ThisClass::OnCreateSession);
	}
	return true;
}

void UMultiplayerHost::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Yellow,
				FString::Printf(TEXT("Session Created successfuly"))
			);
		}
		UWorld* World = GetWorld();
		if (World)
		{
			//World->ServerTravel(FString("/Game/ThirdPerson/Maps/L_Lobby?listen"));
			UGameplayStatics::OpenLevel(GetWorld(), "/Game/Maps/L_Startup", true, "listen");
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString::Printf(TEXT("Session was not Created"))
			);
		}
		HostSessionButton->SetIsEnabled(true);
	}
}

void UMultiplayerHost::HostSessionButtonOnClicked()
{
	FString StringName = SessionNameEText->GetText().ToString();
	FName SessionName = FName(*StringName);
	HostSessionButton->SetIsEnabled(false);
	if (!MultiplayerSessionsSubsystem) { return; }
	//MultiplayerSessionsSubsystem->CreateSession(4/*UKismetStringLibrary::Conv_StringToInt(ComboMaxPlayers->GetSelectedOption())*/, MatchType, SessionName);
}
