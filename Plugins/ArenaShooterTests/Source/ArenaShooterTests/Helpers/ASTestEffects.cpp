#include "Helpers/ASTestEffects.h"

#include "GameplayEffect.h"
#include "AbilitySystem/Attributes/ASCombatAttributeSet.h"
#include "AbilitySystem/Executions/ASDamageExecution.h"

UGameplayEffect* ASTestEffects::MakeRawDamage(float Amount)
{
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UASCombatAttributeSet::GetDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::AddBase;
	Modifier.ModifierMagnitude = FScalableFloat(Amount);
	return Effect;
}

UGameplayEffect* ASTestEffects::MakeExecutionDamage()
{
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayEffectExecutionDefinition& Execution = Effect->Executions.AddDefaulted_GetRef();
	Execution.CalculationClass = UASDamageExecution::StaticClass();
	return Effect;
}