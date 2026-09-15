// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Pickups/ASPickup.h"
#include "ASPickup_Ammo.generated.h"


UCLASS()
class ARENASHOOTER_API AASPickup_Ammo : public AASPickup
{
	GENERATED_BODY()
protected:

	virtual bool GiveTo(AActor* Actor, UAbilitySystemComponent* ASC) override;

	/** Feeds every weapon whose definition carries this tag, or a child of it. */
	UPROPERTY(EditAnywhere, Category = "ArenaShooter|Pickup", meta = (Categories = "Ammo"))
	FGameplayTag AmmoType;

	UPROPERTY(EditAnywhere, Category = "AS|Pickup")
	int32 AmmoAmount = 30;
};
