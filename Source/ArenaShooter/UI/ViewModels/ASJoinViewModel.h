// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ASJoinViewModel.generated.h"

class UMultiplayerSessionsSubsystem;
class UASSessionRowViewModel;
/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASJoinViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Host")
	void Initialize(APlayerController* OwningPC);

	UFUNCTION(BlueprintCallable, Category = "Join")
	void RefreshSessions();

	void JoinResult(const FOnlineSessionSearchResult& Result); // called by a row

	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "Join")
	FString MatchType = TEXT("FreeForAll");
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "Join")
	int32 MaxSearchResults = 10000;

	void SetStatusText(const FText& Value);
	void SetIsBusy(bool Value);

protected:
	virtual void BeginDestroy() override;

private:
	void HandleFindComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful);
	void HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void SetSessions(const TArray<TObjectPtr<UASSessionRowViewModel>>& Value);

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	TArray<TObjectPtr<UASSessionRowViewModel>> Sessions;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	FText StatusText;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	bool bIsBusy = false;

	TWeakObjectPtr<UMultiplayerSessionsSubsystem> SessionsSubsystem;
};
