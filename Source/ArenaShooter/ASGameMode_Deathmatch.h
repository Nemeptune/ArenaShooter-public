// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASGameMode.h"
#include "ASGameMode_Deathmatch.generated.h"

class AASPlayerController;

UCLASS()
class ARENASHOOTER_API AASGameMode_Deathmatch : public AASGameMode
{
	GENERATED_BODY()

public:
	AASGameMode_Deathmatch();

	virtual void PreInitializeComponents() override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	void OnPlayerKilled(AActor* Killer, AActor* Victim);

protected:

	UPROPERTY(EditDefaultsOnly, Category="Match")
	int32 WarmupTime = 180;

	UPROPERTY(EditDefaultsOnly, Category="Match")
	int32 ShortWarmupTime = 10;
	
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 MatchTime = 600.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	float RespawnDelay = 3.f;

private:
	FTimerHandle TimerHandle_DefaultTimer;

	// Pending respawn timers keyed by the dead player's controller, so they can be cancelled
	// (e.g. on Logout or match end) before they fire. Server-only bookkeeping.
	TMap<AController*, FTimerHandle> RespawnTimers;

	void DefaultTimer();
	void RespawnPlayer(TWeakObjectPtr<AController> ControllerPtr);
};
