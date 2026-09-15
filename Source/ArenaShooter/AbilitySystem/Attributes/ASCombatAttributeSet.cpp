// Fill out your copyright notice in the Description page of Project Settings.


#include "ASCombatAttributeSet.h"

#include "Character/ASCharacter.h"
#include "System/ASGameplayTags.h"
#include "System/ASLogChannels.h"
#include "Messages/ASMessageTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Messages/ASDamageMessage.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"

UASCombatAttributeSet::UASCombatAttributeSet()
{
	ShieldAbsorptionPercent = 1.0f;
	InitDamageMultiplier(1.0f);
	InitDamageResistance(0.0f);
}

void UASCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UASCombatAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASCombatAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASCombatAttributeSet, Shield, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASCombatAttributeSet, MaxShield, COND_OwnerOnly, REPNOTIFY_Always);
}

float UASCombatAttributeSet::ASGetHealth() const
{
	return GetHealth();
}

void UASCombatAttributeSet::ASSetHealth(float NewValue)
{
	SetHealth(NewValue);
}

void UASCombatAttributeSet::ASInitHealth(float NewValue)
{
	InitHealth(NewValue);
}

void UASCombatAttributeSet::ASInitMaxHealth(float NewValue)
{
	InitMaxHealth(NewValue);
}

void UASCombatAttributeSet::ASInitShield(float NewValue)
{
	InitShield(NewValue);
}

void UASCombatAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UASCombatAttributeSet, Health, OldValue);
}

void UASCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UASCombatAttributeSet, MaxHealth, OldValue);
}

void UASCombatAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UASCombatAttributeSet, Shield, OldValue);
}

void UASCombatAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UASCombatAttributeSet, MaxShield, OldValue);
}

//called on the server
bool UASCombatAttributeSet::PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		ShieldDamageThisExecution = 0.f;
		if (GetShield() > 0)
		{
			float CapturedDamage = Data.EvaluatedData.Magnitude;
			Data.EvaluatedData.Magnitude *= (1.f - ShieldAbsorptionPercent);
			
			const float Requested = CapturedDamage - Data.EvaluatedData.Magnitude;
			ShieldDamageThisExecution = FMath::Min(Requested, GetShield());
			Data.EvaluatedData.Magnitude += (Requested - ShieldDamageThisExecution);
			
			SetShield(GetShield() - ShieldDamageThisExecution);
		}
	}
	return true;
}

void UASCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	AActor* TargetActor = nullptr;
	AASCharacter* TargetCharacter = nullptr;
	
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetCharacter = Cast<AASCharacter>(TargetActor);
	}
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		bool WasAlive = true;
		if (TargetCharacter)
		{
			WasAlive = TargetCharacter->IsAlive();			
		}
		
		const float HealthBefore = GetHealth();
		
		SetHealth(FMath::Clamp(GetHealth() - Damage.GetCurrentValue(), 0.0f, GetMaxHealth()));
		Damage = 0.0f;
		
		const float TotalDealt = (HealthBefore - GetHealth()) + ShieldDamageThisExecution;
		
		const bool bLethal = WasAlive && TargetCharacter && !TargetCharacter->IsAlive();
		BroadcastDamageDealt(Data, TargetActor, TotalDealt, bLethal);
		
		ApplyDamageReactions(Data, TotalDealt);

		if (bLethal)
		{
			const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
			AActor* Killer = EffectContext.GetEffectCauser();
			AActor* Victim = GetOwningActor();

			// Hand off to the death ability
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				FGameplayEventData Payload;
				Payload.EventTag = FASGameplayTags::Event_Death;
				Payload.Instigator = Killer;
				Payload.Target = Victim;
				Payload.ContextHandle = EffectContext;
				ASC->HandleGameplayEvent(FASGameplayTags::Event_Death, &Payload);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth() + Healing.GetCurrentValue(), 0.0f, GetMaxHealth()));
		Healing = 0.0f;
	}
	else if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Clamp(GetShield(), 0.0f, GetMaxShield()));
	}
}

void UASCombatAttributeSet::BroadcastDamageDealt(const FGameplayEffectModCallbackData& Data, AActor* TargetActor, float TotalDamage, bool bLethal) const
{
	if (TotalDamage <= 0.f || !TargetActor)
	{
		return;
	}
	
	const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetEffectContext();
	
	FASDamageDealtMessage Message;
	Message.Target = TargetActor;
	Message.Damage = TotalDamage;
	Message.Tags = Data.EffectSpec.GetDynamicAssetTags();
	if (bLethal)
	{
		Message.Tags.AddTag(FASGameplayTags::Damage_Lethal);
	}
	Message.Location = TargetActor->GetActorLocation();
	
	if (const FHitResult* Hit = Context.GetHitResult())
	{
		Message.Location = Hit->ImpactPoint;
	}
	
	if (const UAbilitySystemComponent* SourceASC = Context.GetInstigatorAbilitySystemComponent())
	{
		Message.Instigator = Cast<APlayerState>(SourceASC->GetOwnerActor());
	}
	
	if (!Message.Instigator)
	{
		if (AActor* InstigatorActor = Context.GetOriginalInstigator())
		{
			if (APlayerState* PS = Cast<APlayerState>(InstigatorActor))
			{
				Message.Instigator = PS;
			}
			else if (const APawn* Pawn = Cast<APawn>(InstigatorActor))
			{
				Message.Instigator = Pawn->GetPlayerState();
			}
		}
	}
	
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(FASMessageTags::Damage_Dealt, Message);
}

void UASCombatAttributeSet::ApplyDamageReactions(const FGameplayEffectModCallbackData& Data, float TotalDealt) const
{
	if (TotalDealt <= 0.f)
	{
		return;
	}
	
	// A reaction's own damage must not start another round of reactions: two Reflect players
	// would otherwise trade one hit back and forth until somebody dies, inside a single frame.
	if (Data.EffectSpec.GetDynamicAssetTags().HasTagExact(FASGameplayTags::Damage_Reflected))
	{
		return;
	}
	
	UAbilitySystemComponent* VictimASC = GetOwningAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetEffectContext().GetInstigatorAbilitySystemComponent();
	
	// Self-damage is not an exchange between two players.
	if (!VictimASC || !SourceASC || SourceASC == VictimASC)
	{
		return;
	}
	
	FGameplayEventData Payload;
	Payload.EventMagnitude = TotalDealt;
	Payload.Instigator = SourceASC->GetAvatarActor();
	Payload.Target = VictimASC->GetAvatarActor();
	Payload.ContextHandle = Data.EffectSpec.GetEffectContext();
	
	Payload.EventTag = FASGameplayTags::Event_Damage_Dealt;
	SourceASC->HandleGameplayEvent(Payload.EventTag, &Payload);
	
	Payload.EventTag = FASGameplayTags::Event_Damage_Taken;
	VictimASC->HandleGameplayEvent(Payload.EventTag, &Payload);
}
