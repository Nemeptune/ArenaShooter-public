// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/ASPickup.h"
#include "ASPickup_Effect.generated.h"

class UGameplayEffect;

/**
 * All pickups should derive from this class
 */
UCLASS()
class ARENASHOOTER_API AASPickup_Effect : public AASPickup
{
	GENERATED_BODY()
protected:
	UPROPERTY(EditAnywhere, Category = "ArenaShooter|Pickup")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	virtual bool GiveTo(AActor* Actor, UAbilitySystemComponent* ASC) override;
};
