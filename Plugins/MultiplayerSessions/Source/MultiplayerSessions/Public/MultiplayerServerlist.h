// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "MultiplayerServerlist.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerServerlist : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(meta = (BindWidget))
	class UScrollBox* SessionList;

	UPROPERTY(meta = (BindWidget))
	class UButton* RefreshButton;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UMultiplayerSessionLine> SessionLineClass;
public:

protected:

	virtual bool Initialize() override;

	UFUNCTION()
	void RefreshSessions();

	void AddSessionToSessionList(FOnlineSessionSearchResult Result, FString InAndMaxPlayers);

	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);

private:

	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
	FString MatchType{ TEXT("FreeForAll") };
};
