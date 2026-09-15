// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerMenu.generated.h"

class UButton;
/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString LobbyPath = FString(TEXT("/Game/ThirdPerson/Maps/L_Lobby")));
protected:

	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

	//
	// Callbacks for the custom delegates on the MultiplayerSessionsSubsystem
	//
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	//void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	//void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnDestroySession(FName SessionName, bool bWasSuccessful);
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

private:

	/*UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;
	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;*/
	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;


	UFUNCTION()
	void HostButtonOnClicked();
	UFUNCTION()
	void JoinButtonOnClicked();
	UFUNCTION()
	void QuitButtonOnClicked();

	void MenuTearDown();

	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	int32 NumPublicConnections{ 4 };
	FString MatchType{ TEXT("FreeForAll") };
	FString PathToLobby{ TEXT("") };
};
