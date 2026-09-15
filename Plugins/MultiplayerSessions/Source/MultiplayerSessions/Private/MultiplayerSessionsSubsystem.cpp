// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include <Online/OnlineSessionNames.h>

#include "Kismet/GameplayStatics.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType, FName SessionName, TSoftObjectPtr<UWorld> HostDestination)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		return;
	}

	// Re-entrancy guard: a create is already pending -> don't re-add the delegate.
	if (CreateSessionCompleteDelegateHandle.IsValid())
	{
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		return;
	}

	// Null guard: validate before dereferencing below.
	UWorld* World = GetWorld();
	const ULocalPlayer* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		return;
	}

	PendingHostDestination = HostDestination;

	auto ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession != nullptr)
	{
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;
		LastHostDestination = HostDestination;
		SessionInterface->DestroySession(SessionName);
		return;
	}

	//Store the delegate in a FDelegateHandle so we can remove it from delegate list later
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true;
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set(FName("SESSION_NAME"), SessionName.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set(FName("SESSION_MAP"),
		HostDestination.IsNull() ? FString() : HostDestination.ToSoftObjectPath().GetAssetName(),
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1;
	
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), SessionName, *LastSessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	if (FindSessionsCompleteDelegateHandle.IsValid())
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	UWorld* World = GetWorld();
	const ULocalPlayer* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}
	
	//Store the delegate in a FDelegateHandle so we can remove it from delegate list later
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	LastSessionSearch->QuerySettings.Set(FName("MatchType"), FString("FreeForAll"), EOnlineComparisonOp::Equals);

	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult, FName SessionName)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(SessionName, EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	if (JoinSessionCompleteDelegateHandle.IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(SessionName, EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	UWorld* World = GetWorld();
	const ULocalPlayer* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(SessionName, EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	//Store the delegate in a FDelegateHandle so we can remove it from delegate list later
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), SessionName, SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
		MultiplayerOnJoinSessionComplete.Broadcast(SessionName, EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UMultiplayerSessionsSubsystem::DestroySession(FName SessionName)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(SessionName,false);
		FinishLeaveIfPending();
		return;
	}
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	if (!SessionInterface->DestroySession(SessionName))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

		MultiplayerOnDestroySessionComplete.Broadcast(SessionName,false);
		FinishLeaveIfPending();
	}
}

void UMultiplayerSessionsSubsystem::StartSession()
{
}

void UMultiplayerSessionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
	
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UMultiplayerSessionsSubsystem::HandleNetworkFailure);
	}
}

void UMultiplayerSessionsSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}

	SessionInterface.Reset();
	
	Super::Deinitialize();
}

void UMultiplayerSessionsSubsystem::LeaveSessionAndTravel(TSoftObjectPtr<UWorld> Destination)
{
	PendingLeaveDestination = Destination;

	const bool bHasSession = SessionInterface.IsValid() && !ActiveSessionName.IsNone() && SessionInterface->GetNamedSession(ActiveSessionName) != nullptr;

	if (!bHasSession)
	{
		TravelToLeaveDestination();
		return;
	}

	bTravelOnDestroy = true;
	DestroySession(ActiveSessionName);
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	CreateSessionCompleteDelegateHandle.Reset();
	
	if (bWasSuccessful)
	{
		ActiveSessionName = SessionName;
	}
	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);

	if (bWasSuccessful && !PendingHostDestination.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, PendingHostDestination, true, FString("listen"));
	}
	PendingHostDestination.Reset();
}

void UMultiplayerSessionsSubsystem::OnFindSessionComplete(bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle); 
	}

	FindSessionsCompleteDelegateHandle.Reset();
	
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		//Empty array for 0 results
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}
	//giving array of results to menu
	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

// we don't need session name, all we do is passing result of connection
void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName,EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	JoinSessionCompleteDelegateHandle.Reset();
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Yellow,
			FString::Printf(TEXT("OnJoinSessionComplete"))
		);
	}

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		ActiveSessionName = SessionName;

		FString Adress;
		if (SessionInterface.IsValid() && SessionInterface->GetResolvedConnectString(SessionName, Adress))
		{
			if (APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr)
			{
				PC->ClientTravel(Adress, TRAVEL_Absolute);
			}
		}
	}
	
	MultiplayerOnJoinSessionComplete.Broadcast(SessionName, Result);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	DestroySessionCompleteDelegateHandle.Reset();
	
	if (bWasSuccessful && SessionName == ActiveSessionName)
	{
		ActiveSessionName = NAME_None;
	}
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType, SessionName, LastHostDestination);
	}
	MultiplayerOnDestroySessionComplete.Broadcast(SessionName,bWasSuccessful);
	FinishLeaveIfPending();
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}

void UMultiplayerSessionsSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogTemp, Warning, TEXT("Network failure %s: %s"), ENetworkFailure::ToString(FailureType), *ErrorString);
	LeaveSessionAndTravel(TSoftObjectPtr<UWorld>(MenuLevelPath));
}

void UMultiplayerSessionsSubsystem::FinishLeaveIfPending()
{
	if (bTravelOnDestroy)
	{
		bTravelOnDestroy = false;
		// Travel even if the destroy failed
		TravelToLeaveDestination();
	}
}

void UMultiplayerSessionsSubsystem::TravelToLeaveDestination()
{
	if (!PendingLeaveDestination.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, PendingLeaveDestination);
		PendingLeaveDestination.Reset();
	}
}
