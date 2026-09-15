// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ASGameplayAbility.h"
#include "ASGameplayAbility_Death.generated.h"

/**
 * Server-only ability that coordinates a character's death.
 *
 * - Triggered by the Event.Death gameplay event sent from the attribute set on lethal damage.
 * - Holds Status.Death.Dead via ActivationOwnedTags for the entire duration the avatar is dead.
 * - Tells the character to enter its physical death state (ragdoll via replicated bIsDead).
 * - Notifies the game mode so it can credit the kill and schedule a respawn.
 * - Stays active until it receives Event.Character.Respawned, then ends (releasing the dead tag).
 */
UCLASS()
class ARENASHOOTER_API UASGameplayAbility_Death : public UASGameplayAbility
{
	GENERATED_BODY()

public:
	UASGameplayAbility_Death();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// Called when the game mode signals the player has respawned. Ends the ability.
	UFUNCTION()
	void OnRespawnEventReceived(FGameplayEventData Payload);
};
