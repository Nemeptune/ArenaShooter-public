// Fill out your copyright notice in the Description page of Project Settings.


#include "ASDamageExecution.h"

#include "AbilitySystem/Attributes/ASCombatAttributeSet.h"
#include "System/ASGameplayTags.h"
#include "System/ASProfiling.h"

namespace
{
	struct FASDamageStatics
	{
		FGameplayEffectAttributeCaptureDefinition  DamageMultiplierDef;
		FGameplayEffectAttributeCaptureDefinition  DamageResistanceDef;
		
		FASDamageStatics()
			: DamageMultiplierDef(
			UASCombatAttributeSet::GetDamageMultiplierAttribute(),
			EGameplayEffectAttributeCaptureSource::Source, true)
			, DamageResistanceDef(
			UASCombatAttributeSet::GetDamageResistanceAttribute(),
			EGameplayEffectAttributeCaptureSource::Target, true)
		{}
	};

	const FASDamageStatics& DamageStatics()
	{
		static FASDamageStatics Statics;
		return Statics;
	}
}

UASDamageExecution::UASDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().DamageMultiplierDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamageResistanceDef);
}

void UASDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASDamageExecution::Execute_Implementation);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_Damage);
	
	CSV_CUSTOM_STAT(ArenaShooter, DamageExecutions, 1, ECsvCustomStatOp::Accumulate);
	
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	const float Base = Spec.GetSetByCallerMagnitude(FASGameplayTags::Data_Damage, false, 0.f);
	if (Base <= 0.f)
	{
		return;
	}
	
	float SourceMultiplier = 1.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageMultiplierDef, EvalParams, SourceMultiplier);
	SourceMultiplier = FMath::Max(0.f, SourceMultiplier);
	
	float TargetResistance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageResistanceDef, EvalParams, TargetResistance);
	TargetResistance = FMath::Clamp(TargetResistance, 0.f, 1.f);
	
	const float FinalDamage = Base * SourceMultiplier * (1.f - TargetResistance);
	
	if (FinalDamage > 0.f)
	{
		UE_LOG(LogTemp, Verbose, TEXT("DamageExec: Base=%.1f SrcMult=%.2f TgtResist=%.2f Final=%.1f"),
	Base, SourceMultiplier, TargetResistance, FinalDamage);
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UASCombatAttributeSet::GetDamageAttribute(),
			EGameplayModOp::AddBase,
			FinalDamage));
	}
}
