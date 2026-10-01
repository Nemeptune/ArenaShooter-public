#include "CQTest.h"
#include "Components/ActorTestSpawner.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Attributes/ASCombatAttributeSet.h"
#include "Helpers/ASTestEffects.h"
#include "Helpers/ASTestActors.h"
#include "System/ASGameplayTags.h"

namespace
{
	constexpr float AttributeTolerance = 1e-3f;
}

TEST_CLASS(FASDamageTests, "ArenaShooter.AbilitySystem.Damage")
{
	FActorTestSpawner Spawner;
	AASTestAbilityActor* Attacker = nullptr;
	AASTestAbilityActor* Victim = nullptr;

	int32 AttackerDealtEvents = 0;
	int32 VictimDealtEvents = 0;
	int32 VictimTakenEvents = 0;

	BEFORE_EACH()
	{
		// Every hit is broadcast on the gameplay message router, which lives on the game instance.
		Spawner.InitializeGameSubsystems();

		Attacker = &Spawner.SpawnActor<AASTestAbilityActor>();
		Victim = &Spawner.SpawnActor<AASTestAbilityActor>();

		Victim->Attributes->InitMaxHealth(100.f);
		Victim->Attributes->InitHealth(100.f);
		Victim->Attributes->InitMaxShield(100.f);
		Victim->Attributes->InitShield(0.f);

		const FGameplayTag DealtTag = FASGameplayTags::Event_Damage_Dealt;
		const FGameplayTag TakenTag = FASGameplayTags::Event_Damage_Taken;
		Attacker->AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(DealtTag).AddLambda([this](const FGameplayEventData*) { ++AttackerDealtEvents; });
		Victim->AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(DealtTag).AddLambda([this](const FGameplayEventData*) { ++VictimDealtEvents; });
		Victim->AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(TakenTag).AddLambda([this](const FGameplayEventData*) { ++VictimTakenEvents; });
	}

	void HitRaw(AASTestAbilityActor& From, AASTestAbilityActor& To, float Amount, FGameplayTag ExtraAssetTag = FGameplayTag())
	{
		FGameplayEffectSpec Spec(ASTestEffects::MakeRawDamage(Amount), From.AbilitySystemComponent->MakeEffectContext(), 1.f);
		if (ExtraAssetTag.IsValid())
		{
			Spec.AddDynamicAssetTag(ExtraAssetTag);
		}
		From.AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(Spec, To.AbilitySystemComponent);
	}

	void HitThroughExecution(float BaseDamage)
	{
		// Created after the attacker's multiplier is set: the spec snapshots it here.
		FGameplayEffectSpec Spec(ASTestEffects::MakeExecutionDamage(), Attacker->AbilitySystemComponent->MakeEffectContext(), 1.f);
		Spec.SetSetByCallerMagnitude(FASGameplayTags::Data_Damage, BaseDamage);
		Attacker->AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(Spec, Victim->AbilitySystemComponent);
	}

	TEST_METHOD(Shield_AbsorbsDamageBeforeHealth)
	{
		Victim->Attributes->InitShield(50.f);
		HitRaw(*Attacker, *Victim, 30.f);
		ASSERT_THAT(IsNear(20.f, Victim->Attributes->GetShield(), AttributeTolerance));
		ASSERT_THAT(IsNear(100.f, Victim->Attributes->GetHealth(), AttributeTolerance));
	}

	TEST_METHOD(DamagePastTheShield_ReachesHealth)
	{
		Victim->Attributes->InitShield(20.f);
		HitRaw(*Attacker, *Victim, 50.f);
		ASSERT_THAT(IsNear(0.f, Victim->Attributes->GetShield(), AttributeTolerance));
		ASSERT_THAT(IsNear(70.f, Victim->Attributes->GetHealth(), AttributeTolerance));
	}

	TEST_METHOD(Health_NeverDropsBelowZero)
	{
		HitRaw(*Attacker, *Victim, 500.f);
		ASSERT_THAT(IsNear(0.f, Victim->Attributes->GetHealth(), AttributeTolerance));
	}

	TEST_METHOD(DamageMetaAttribute_IsClearedAfterTheHit)
	{
		HitRaw(*Attacker, *Victim, 25.f);
		ASSERT_THAT(IsNear(0.f, Victim->Attributes->GetDamage(), AttributeTolerance));
	}

	TEST_METHOD(Execution_PassesBaseDamageThrough)
	{
		HitThroughExecution(40.f);
		ASSERT_THAT(IsNear(60.f, Victim->Attributes->GetHealth(), AttributeTolerance));
	}

	TEST_METHOD(Execution_AttackerMultiplierScalesDamage)
	{
		Attacker->Attributes->SetDamageMultiplier(2.f);
		HitThroughExecution(40.f);
		ASSERT_THAT(IsNear(20.f, Victim->Attributes->GetHealth(), AttributeTolerance));
	}

	TEST_METHOD(Execution_VictimResistanceReducesDamage)
	{
		Victim->Attributes->SetDamageResistance(0.25f);
		HitThroughExecution(40.f);
		ASSERT_THAT(IsNear(70.f, Victim->Attributes->GetHealth(), AttributeTolerance));
	}

	TEST_METHOD(Execution_ResistanceAboveOne_BlocksAllDamageButNeverHeals)
	{
		Victim->Attributes->SetDamageResistance(1.5f);
		HitThroughExecution(40.f);
		ASSERT_THAT(IsNear(100.f, Victim->Attributes->GetHealth(), AttributeTolerance));
	}

	TEST_METHOD(Execution_NegativeMultiplier_DealsNothingInsteadOfHealing)
	{
		Attacker->Attributes->SetDamageMultiplier(-1.f);
		Victim->Attributes->InitHealth(50.f);
		HitThroughExecution(40.f);
		ASSERT_THAT(IsNear(50.f, Victim->Attributes->GetHealth(), AttributeTolerance));
	}

	TEST_METHOD(Execution_WithoutSetByCallerDamage_DoesNothing)
	{
		HitThroughExecution(0.f);
		ASSERT_THAT(IsNear(100.f, Victim->Attributes->GetHealth(), AttributeTolerance));
	}

	TEST_METHOD(Hit_NotifiesAttackerAndVictimOnce)
	{
		HitRaw(*Attacker, *Victim, 10.f);
		ASSERT_THAT(AreEqual(1, AttackerDealtEvents));
		ASSERT_THAT(AreEqual(1, VictimTakenEvents));
	}

	TEST_METHOD(ReflectedHit_DealsDamageButStartsNoReactions)
	{
		// Otherwise two Reflect players would bounce one hit back and forth until someone died, in a single frame.
		HitRaw(*Attacker, *Victim, 10.f, FASGameplayTags::Damage_Reflected);
		ASSERT_THAT(IsNear(90.f, Victim->Attributes->GetHealth(), AttributeTolerance));
		ASSERT_THAT(AreEqual(0, AttackerDealtEvents));
		ASSERT_THAT(AreEqual(0, VictimTakenEvents));
	}

	TEST_METHOD(SelfDamage_StartsNoReactions)
	{
		HitRaw(*Victim, *Victim, 10.f);
		ASSERT_THAT(IsNear(90.f, Victim->Attributes->GetHealth(), AttributeTolerance));
		ASSERT_THAT(AreEqual(0, VictimDealtEvents));
		ASSERT_THAT(AreEqual(0, VictimTakenEvents));
	}
};