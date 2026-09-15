// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Messages/ASDamageMessage.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "ASNumberPopRelay.generated.h"


UCLASS()
class ARENASHOOTER_API UASNumberPopRelay : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	
private:
	struct FPendingPop
	{
		TWeakObjectPtr<AActor> Target;
		FVector Location = FVector::ZeroVector;
		float Damage = 0.f;
		FGameplayTagContainer Tags;
	};
	
	void OnDamageDealt(FGameplayTag Channel, const FASDamageDealtMessage& Message);
	void Flush();
	
	FGameplayMessageListenerHandle ListenerHandle;
	TMap<TWeakObjectPtr<APlayerController>, TArray<FPendingPop>> Pending;
	bool bFlushScheduled = false;
};
