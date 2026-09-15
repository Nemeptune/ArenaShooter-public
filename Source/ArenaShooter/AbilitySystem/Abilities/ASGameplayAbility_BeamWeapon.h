// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASGameplayAbility_FromEquipment.h"
#include "ASGameplayAbility_BeamWeapon.generated.h"

/**
* Continuous beam weapon ability (railgun). Server-authoritative: the server
 * samples the beam on a fast timer and applies instant damage the moment the
 * crosshair is on a target. A short-duration "cooldown" GE grants a tag on each
 * victim, gating that victim to one hit per cooldown duration
 */
UCLASS()
class ARENASHOOTER_API UASGameplayAbility_BeamWeapon : public UASGameplayAbility_FromEquipment
{
	GENERATED_BODY()

public:
	UASGameplayAbility_BeamWeapon();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Beam")
	TSubclassOf<UGameplayEffect> HitCooldownEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Beam")
	FGameplayTag HitCooldownTag;

	// rate of beam detection
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Beam")
	float SampleInterval = 0.033f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Beam|Debug")
	bool bDrawDebugTrace = false;

	// ammo drain
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Beam")
	float CostInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Beam")
	float AnglePerSubstepDeg = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Beam")
	int32 MaxSubsteps = 16;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Beam")
	FGameplayTag BeamCueTag;

	UFUNCTION()
	void OnInputReleased(float TimeHeld);

private:
	void HandleBeamTick();
	bool PerformServerSweptTrace(FHitResult& OutResult);
	void ApplyDamage(const FHitResult& HitResult);

	FTimerHandle SampleTimerHandle;
	float LastCostTime = 0.f;

	FVector PrevAimDir = FVector::ZeroVector;
	bool bHasPrevAim = false;
};
