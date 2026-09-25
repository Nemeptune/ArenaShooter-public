// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ASGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSimpleDynamicMulticastDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FASPlayerStateChanged, APlayerState*);

UCLASS()
class ARENASHOOTER_API AASGameState : public AGameState
{
	GENERATED_BODY()

public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnPlayerJoined();
	
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category="Match")
	int32 RemainingTime = 0;
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category="Match")
	bool bTimerPaused = false;

	FASPlayerStateChanged OnPlayerAdded;
	FASPlayerStateChanged OnPlayerRemoved;

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void OnRep_MatchState() override;

	void SetMatchResult(APlayerState* InWinner);

	APlayerState* GetWinner() const;
	bool HasMatchEnded() const;
	
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void BroadcastMatchEnded() const;

	UFUNCTION()
	void OnRep_MatchEnded();
	
	void EndMatchStateRegion();

	/** null = draw */
	UPROPERTY(Transient, Replicated)
	TObjectPtr<APlayerState> WinnerPS;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_MatchEnded)
	bool bMatchEnded = false;
	
	UPROPERTY(BlueprintAssignable)
	FSimpleDynamicMulticastDelegate OnPlayerJoined;
	
	/** Insights region for the current match state, so a trace can be split by phase. 0 = none open. */
	uint64 MatchStateRegionId = 0;
};
