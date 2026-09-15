// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "MultiplayerSessionLine.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionLine : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* SessionNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayersInSessionText;

	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;

	const FName StoredSessionName;
	FOnlineSessionSearchResult SessionResult;

public:

	void SetupSessionLineText(FOnlineSessionSearchResult Result, FString InAndMaxPlayers);

protected:
	virtual bool Initialize() override;
	void OnJoinSession(FName SessionName,EOnJoinSessionCompleteResult::Type Result);
private:

	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	UFUNCTION()
	void JoinButtonOnClicked();
	
};
