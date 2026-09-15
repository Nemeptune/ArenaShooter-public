// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "ASScoreViewModel.generated.h"

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASScoreViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	void SetLocalKills(int32 Value);
	void SetOpponentKills(int32 Value);
	void SetLocalName(const FText& Value);
	void SetOpponentName(const FText& Value);
	void SetMatchTimeText(const FText& Value);
	void SetLocalAvatar(UTexture2D* Value);
	void SetOpponentAvatar(UTexture2D* Value);

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	int32 LocalKills = 0;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	int32 OpponentKills = 0;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	FText LocalName;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	FText OpponentName;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	FText MatchTimeText;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	TObjectPtr<UTexture2D> LocalAvatar;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess))
	TObjectPtr<UTexture2D> OpponentAvatar;
};
