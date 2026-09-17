// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASPlayerControllerBase.h"
#include "ASPlayerController.generated.h"

#define NULL_PROJECTILE_ID 0

class UInputMappingContext;
struct FASNumberPop;
class AASProjectile;
class UInputAction;
class UASAbilitySystemComponent;
struct FGameplayTag;
class UASInputConfig;

/**
 * 
 */
UCLASS(Config=Game)
class ARENASHOOTER_API AASPlayerController : public AASPlayerControllerBase
{
	GENERATED_BODY()
	
public:
	AASPlayerController();
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	void SetAbilitySystemComponent(UASAbilitySystemComponent* ASC);

	// --- Settings for predicting projectile weapons ---
	//Used to "fudge" the server's estimate of a client's ping to get a more accurate guess of their exact ping.
	UPROPERTY(BlueprintReadOnly, Config, Category = "Network")
	float PredictionLatencyReduction;

	/** How much (from 0-1) to favor the client when determining the real position of predicted projectiles. A greater
	* value will spawn authoritative projectiles closer to where the client wants; a smaller value will spawn
	* authoritative projectiles closer to where the server wants (forwarding them less). */
	UPROPERTY(BlueprintReadOnly, Config, Category = "Network")
	float ClientBiasPct;

	/** Max amount of ping to predict ahead for. If the client's ping exceeds this, we'll delay spawning the projectile
	* so it doesn't spawn further ahead than this. */
	UPROPERTY(BlueprintReadOnly, Config, Category = "Network")
	float MaxPredictionPing;

	/** The amount of time, in seconds, to tick or simulate to make up for network lag. (1/2 player's ping) - prediction
	* latency reduction. */
	float GetForwardPredictionTime() const;

	/** How long to wait before spawning the projectile if the client's ping exceeds MaxPredictionPing, so we don't
	* forward-predict too far. */
	float GetProjectileSleepTime() const;

	/** List of this client's fake projectiles (client-side predicted projectiles) that haven't been linked to an
	 * authoritative projectile yet. */
	UPROPERTY()
	TMap<uint32, TObjectPtr<AASProjectile>> FakeProjectiles;

	uint32 GenerateNewFakeProjectileID();
	
	UFUNCTION(Client, Unreliable)
	void ClientNumberPops(const TArray<FASNumberPop>& Pops);
	
	// Pawn-independent keys (game menu). Added once by the controller, never cleared by pawns.
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> GlobalMappingContext;
	
protected:
	UPROPERTY()
	TObjectPtr<UASAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UASInputConfig> AbilityInputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> OpenGameMenuAction;

	void HandleOpenGameMenu();
	
	// Server only
	virtual void OnPossess(APawn *InPawn) override;

	void AbilityInputPressed(FGameplayTag InputTag);
	void AbilityInputReleased(FGameplayTag InputTag);
	
	void SendInputEvent(FGameplayTag EventTag);

private:
	uint32 FakeProjectileIdCounter;
};
