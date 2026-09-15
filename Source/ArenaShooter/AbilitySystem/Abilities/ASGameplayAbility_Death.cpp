// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameplayAbility_Death.h"

#include "ASCharacter.h"
#include "ASGameplayTags.h"
#include "ASGameMode_Deathmatch.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UASGameplayAbility_Death::UASGameplayAbility_Death()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;

	// Death is a definitive server-side state change. We never want a client to predict it.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// While this ability is active the avatar is considered dead. GAS applies/removes this tag
	// automatically with the ability's lifetime, so we never have to juggle it by hand.
	ActivationOwnedTags.AddTag(FASGameplayTags::Status_Death_Dead);

	// Auto-activate when the attribute set sends the death event on lethal damage.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = FASGameplayTags::Event_Death;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UASGameplayAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AASCharacter* Victim = GetASCharacterFromActorInfo();
	UASAbilitySystemComponent* ASC = GetASAbilitySystemComponentFromActorInfo();
	
	if (!Victim || !ASC || !Victim->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Cancel every other ability on this avatar but keep ourselves alive.
	ASC->CancelAllAbilities(this);
	
	ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(FASGameplayTags::Buff));
	
	AActor* KillerActor = nullptr;
	if (TriggerEventData)
	{
		KillerActor = const_cast<AActor*>(TriggerEventData->Instigator.Get());
		if (!KillerActor && TriggerEventData->ContextHandle.IsValid())
		{
			KillerActor = TriggerEventData->ContextHandle.GetEffectCauser();
		}
	}

	// Knockback: push the corpse the way the shot was travelling. Prefer the hit result's trace
	// direction, fall back to the surface normal, then away from the killer.
	FVector ImpulseDir = FVector::ZeroVector;
	FVector ImpulseLocation = Victim->GetActorLocation();
	FName ImpulseBone = NAME_None;

	if (TriggerEventData && TriggerEventData->ContextHandle.IsValid())
	{
		if (const FHitResult* Hit = TriggerEventData->ContextHandle.GetHitResult())
		{
			ImpulseLocation = Hit->ImpactPoint;
			ImpulseBone = Hit->BoneName;

			const FVector ShotDir = Hit->TraceEnd - Hit->TraceStart;
			if (!ShotDir.IsNearlyZero())
			{
				ImpulseDir = ShotDir.GetSafeNormal();
			}
			else if (!Hit->ImpactNormal.IsNearlyZero())
			{
				ImpulseDir = Hit->ImpactNormal;
			}
		}
	}

	if (ImpulseDir.IsNearlyZero() && KillerActor)
	{
		ImpulseDir = (Victim->GetActorLocation() - KillerActor->GetActorLocation()).GetSafeNormal();
	}
	
	Victim->StartDeath(ImpulseDir, ImpulseLocation, ImpulseBone);

	// Score + respawn scheduling
	if (AASGameMode_Deathmatch* GM = Victim->GetWorld()->GetAuthGameMode<AASGameMode_Deathmatch>())
	{
		GM->OnPlayerKilled(KillerActor, Victim);
	}

	// Stay active (holding Status.Death.Dead) until the game mode tells us we've respawned.
	UAbilityTask_WaitGameplayEvent* WaitRespawn = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FASGameplayTags::Event_Respawn);
	WaitRespawn->EventReceived.AddDynamic(this, &UASGameplayAbility_Death::OnRespawnEventReceived);
	WaitRespawn->ReadyForActivation();
}

void UASGameplayAbility_Death::OnRespawnEventReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
