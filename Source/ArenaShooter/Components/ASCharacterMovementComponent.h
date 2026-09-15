// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ASCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UASCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "ArenaShooter|Knockback")
	void AddDampedImpulse(FVector Impulse, bool bSelfInflicted);

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Knockback")
	float KnockbackDampingSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Knockback")
	float MaxKnockbackHorizontalVelocity = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Knockback")
	float MaxUndampedImpulseZ = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Knockback")
	float MaxAdditiveKnockbackZ = 750.f;

protected:

	void ForceClientAdjustment();
};
