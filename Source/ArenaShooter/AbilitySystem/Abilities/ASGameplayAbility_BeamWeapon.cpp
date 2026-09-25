// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ASGameplayAbility_BeamWeapon.h"

#include "ArenaShooter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "System/ASLogChannels.h"
#include "Weapon/ASLagCompensationSubsystem.h"
#include "Weapon/ASWeaponInstance.h"

UASGameplayAbility_BeamWeapon::UASGameplayAbility_BeamWeapon()
{
}

void UASGameplayAbility_BeamWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (BeamCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.SourceObject = GetCurrentSourceObject(); // weapon
		CueParams.Instigator = GetAvatarActorFromActorInfo();
		K2_AddGameplayCueWithParams(BeamCueTag, CueParams, true);
	}

	// OnBeamStarted

	if (UAbilityTask_WaitInputRelease* WaitRelease = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false))
	{
		WaitRelease->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
		WaitRelease->ReadyForActivation();
	}


	if (UWorld* World = GetWorld())
	{
		LastCostTime = World->GetTimeSeconds() - CostInterval; // charge on first sample
		World->GetTimerManager().SetTimer(SampleTimerHandle,this, &ThisClass::HandleBeamTick, SampleInterval, true, 0.0f);
	}

}

void UASGameplayAbility_BeamWeapon::HandleBeamTick()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASGameplayAbility_BeamWeapon::HandleBeamTick);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_WeaponFire);
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const float Now = World->GetTimeSeconds();

	if (Now - LastCostTime >= CostInterval)
	{
		LastCostTime = Now;
		if (!CheckCost(CurrentSpecHandle, CurrentActorInfo))
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			return;
		}
		ApplyCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
	}
	
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	FHitResult Hit;
	const bool bHit = PerformServerSweptTrace(Hit);
	UE_LOG(LogAS_Weapon, Verbose, TEXT("Beam sample: %s"), bHit ? *GetNameSafe(Hit.GetActor()) : TEXT("miss"));
	if (bHit)
	{
		ApplyDamage(Hit);
	}
}

bool UASGameplayAbility_BeamWeapon::PerformServerSweptTrace(FHitResult& OutResult)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASGameplayAbility_BeamWeapon::PerformServerSweptTrace);
	
	OutResult = FHitResult();

	AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	const UASLagCompensationSubsystem* LagCompensation = World ? World->GetSubsystem<UASLagCompensationSubsystem>() : nullptr;
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!Avatar || !LagCompensation || !GetWeaponViewpoint(ViewLocation, ViewRotation))
	{
		bHasPrevAim = false;
		return false;
	}

	const FVector Start = ViewLocation;
	const FVector CurrDir = ViewRotation.Vector();
	const FVector FromDir = bHasPrevAim ? PrevAimDir : CurrDir;
	PrevAimDir = CurrDir;
	bHasPrevAim = true;

	// Fan sub-rays only if aim actually moved since last sample; steady aim = one trace.
	const float Dot = FMath::Clamp(FVector::DotProduct(FromDir, CurrDir), -1.f, 1.f);
	int32 NumSteps = 1;
	if (FromDir != CurrDir && Dot < 0.99999f)
	{
		const float SweepAngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
		NumSteps = FMath::Clamp(FMath::CeilToInt(SweepAngleDeg / FMath::Max(AnglePerSubstepDeg, 0.1f)), 1, MaxSubsteps);
	}
	const int32 StartIndex = (NumSteps > 1) ? 0 : 1; // i==0 is FromDir: skip when identical to CurrDir

	TArray<FASShotRay> Rays;
	for (int32 i = StartIndex; i <= NumSteps; ++i)
	{
		const float Alpha = static_cast<float>(i) / static_cast<float>(NumSteps);
		const FVector Dir = FMath::Lerp(FromDir, CurrDir, Alpha).GetSafeNormal();
		if (!Dir.IsNearlyZero())
		{
			Rays.Add({ Start, Start + Dir * TraceRange });
		}
	}

	TArray<FHitResult> Hits;
	Hits.SetNum(Rays.Num());
	LagCompensation->LineTraceRewound(Rays, COLLISION_WEAPON, MakeWeaponTraceParams(), Avatar, Hits);

	bool bHit = false;
	FVector::FReal BestDistSq = TNumericLimits<FVector::FReal>::Max();
	for (int32 i = 0; i < Rays.Num(); ++i)
	{
		const FHitResult& StepHit = Hits[i];
		UAbilitySystemComponent* HitASC = StepHit.bBlockingHit ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(StepHit.GetActor()) : nullptr;
		if (HitASC)
		{
			const FVector::FReal DistSq = FVector::DistSquared(Start, StepHit.ImpactPoint);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				OutResult = StepHit;
				bHit = true;
			}
		}
		
		// green = hit a damageable target, yellow = hit world geo, red = hit nothing
#if ENABLE_DRAW_DEBUG
		if (bDrawDebugTrace)
		{
			const FVector LineEnd = StepHit.bBlockingHit ? FVector(StepHit.ImpactPoint) : Rays[i].End;
			
			const FColor LineColor = HitASC ? FColor::Green : (StepHit.bBlockingHit ? FColor::Yellow : FColor::Red);
			DrawDebugLine(World, Start, LineEnd, LineColor, false, 0.f, 0, 1.f);
			if (StepHit.bBlockingHit)
			{
				DrawDebugPoint(World, StepHit.ImpactPoint, 8.f, LineColor, false, 0.f);
			}
		}
#endif
	}

	return bHit;
}

void UASGameplayAbility_BeamWeapon::ApplyDamage(const FHitResult& HitResult)
{
	const UASWeaponInstance* Weapon = GetSourceWeapon();
	if (!Weapon || !Weapon->CanFire())
	{
		return;
	}

	if (ApplyDamageEffectToTargets(MakeArrayView(&HitResult, 1)) == 0)
	{
		return;
	}

	NotifyWeaponFired();
}

void UASGameplayAbility_BeamWeapon::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SampleTimerHandle);
	}
	
	//OnBeamStopped
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UASGameplayAbility_BeamWeapon::OnInputReleased(float TimeHeld)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
