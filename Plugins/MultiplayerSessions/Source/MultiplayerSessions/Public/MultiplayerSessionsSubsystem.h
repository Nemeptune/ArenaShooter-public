// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.generated.h"

//
// Delegates for menu class to bind callbacks to
//

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults,bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnJoinSessionComplete, FName SessionName, EOnJoinSessionCompleteResult::Type);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnDestroySessionComplete,FName, SessionName, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);

/**
 * 
 */
UCLASS(BlueprintType, Config=Game)
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:

	UMultiplayerSessionsSubsystem();

	//
	// To handle session functionality from menu
	//
	void CreateSession(int32 NumPublicConnections, FString MatchType, FName SessionName, TSoftObjectPtr<UWorld> HostDestination = nullptr);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult, FName SessionName);
	void DestroySession(FName SessionName);
	void StartSession();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	void LeaveSessionAndTravel(TSoftObjectPtr<UWorld> Destination);

	//
	// Custom delegates
	//
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

protected:

	//
	// Internal callbacks for delegated that will be added to Session Interface delegate list.
	// These don't need to be called outside of this class.
	//
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName,EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	//
	// To add to Online Session Interface delegate list.
	// Multiplayer Sessions Subsystem internal callbacks are binded to these
	//
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;

	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	//
	//maybe not useful for unique named sessions
	// 
	// create session after destroying
	bool bCreateSessionOnDestroy{ false };

	//handle info for creating session after destroying
	int32 LastNumPublicConnections;
	FString LastMatchType;
	TSoftObjectPtr<UWorld> PendingHostDestination;
	TSoftObjectPtr<UWorld> LastHostDestination;

	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void FinishLeaveIfPending();
	void TravelToLeaveDestination();
	
	FName ActiveSessionName;

	bool bTravelOnDestroy = false;
	TSoftObjectPtr<UWorld> PendingLeaveDestination;
	FDelegateHandle NetworkFailureHandle;
	
	UPROPERTY(Config)
	FSoftObjectPath MenuLevelPath;
};
