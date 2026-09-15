// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASBinderBase.h"
#include "ASScoreBinder.generated.h"

class UASAvatarSubsystem;
class AASGameState;
class AASPlayerState;
class UASScoreViewModel;
class UTexture2D;

UCLASS()
class ARENASHOOTER_API UASScoreBinder : public UASBinderBase
{
	GENERATED_BODY()

public:
	void Init(AASPlayerState* InLocalPS, UASScoreViewModel* InViewModel);
	
protected:
	virtual void SubscribeAll() override;
	virtual void UnSubscribeAll() override;
	virtual void RefreshAll() override;

private:
	void BindWorld();

	void HandlePlayerAdded(APlayerState* PlayerState);
	void HandlePlayerRemoved(APlayerState* PlayerState);
	void UpdateTimer();
	
	UASAvatarSubsystem* GetAvatarSubsystem() const;
	void RefreshAvatars();
	
	void HandleAvatarReady(uint64 PlatformUserId, UTexture2D* Avatar);

	UFUNCTION()
	void HandleLocalNameChanged();
	UFUNCTION()
	void HandleOpponentNameChanged();
	UFUNCTION()
	void HandleLocalKills(int32 NewKills);
	UFUNCTION()
	void HandleOpponentKills(int32 NewKills);
	UFUNCTION()
	void HandleUniqueIdChanged();

	UPROPERTY()
	TObjectPtr<UASScoreViewModel> ViewModel;
	UPROPERTY()
	TObjectPtr<AASPlayerState> LocalPS;
	UPROPERTY()
	TObjectPtr<AASPlayerState> OpponentPS;
	TWeakObjectPtr<AASGameState> BoundGS;
	TWeakObjectPtr<UASAvatarSubsystem> BoundAvatars;
	
	FDelegateHandle AvatarHandle;
	FTimerHandle Timer;
	bool bWorldBound = false;
};
