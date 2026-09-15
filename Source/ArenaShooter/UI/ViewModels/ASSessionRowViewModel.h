// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "OnlineSessionSettings.h"
#include "ASSessionRowViewModel.generated.h"

class UASJoinViewModel;
/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASSessionRowViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void Setup(UASJoinViewModel* InOwner, const FOnlineSessionSearchResult& InResult, const FText& InName, const FText& InMap, const FText& InPlayers);

	UFUNCTION(BlueprintCallable, Category = "SessionRow")
	void Join();

	void SetSessionName(const FText& Value);
	void SetMapName(const FText& Value);
	void SetPlayerText(const FText& Value);
	void SetPingText(const FText& Value);

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	FText SessionName;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	FText MapName;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	FText PlayersText;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess))
	FText PingText;

	TWeakObjectPtr<UASJoinViewModel> Owner;
	FOnlineSessionSearchResult Result;
};
