// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ASGameplayAbility_WeaponBase.h"
#include "ASGameplayAbility_ShotgunWeapon.generated.h"

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASGameplayAbility_ShotgunWeapon : public UASGameplayAbility_WeaponBase
{
	GENERATED_BODY()

protected:
	virtual void PerformLocalTargeting(TArray<FHitResult>& OutHits) override;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Shotgun", meta = (ClampMin = 1))
	int32 PelletCount = 8;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Shotgun", meta = (ClampMin = 0.0))
	float SpreadHalfAngleDeg = 4.f;
};
