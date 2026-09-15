// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASGameplayAbility_FromEquipment.h"
#include "ASGameplayAbility_ProjectileWeapon.generated.h"


class AASProjectile;

UCLASS()
class ARENASHOOTER_API UASGameplayAbility_ProjectileWeapon : public UASGameplayAbility_FromEquipment
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Weapon")
	void GetProjectileFireTransform(FVector& OutLocation, FRotator& OutRotation) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Weapon|Projectile")
	FVector FireOffset = FVector(0.0f, 0.0f, 0.0f);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Weapon|Projectile")
	TSubclassOf<AASProjectile> ProjectileClass;
};
