// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ASGameplayAbility_BeamWeapon.h"

#include "ArenaShooter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
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
	if (PerformServerSweptTrace(Hit))
	{
		ApplyDamage(Hit);
	}
}

bool UASGameplayAbility_BeamWeapon::PerformServerSweptTrace(FHitResult& OutResult)
{
	OutResult = FHitResult();

	AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!Avatar || !World)
	{
		bHasPrevAim = false;
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;

	if (!GetWeaponViewpoint(ViewLocation, ViewRotation))
	{
		bHasPrevAim = false;
		return false;
	}

	const FVector Start = ViewLocation;
	const FVector CurrDir = ViewRotation.Vector();
	const FVector FromDir = bHasPrevAim ? PrevAimDir : CurrDir;

	// Fan sub-rays only if aim actually moved since last sample; steady aim = one trace.
	const float Dot = FMath::Clamp(FVector::DotProduct(FromDir, CurrDir), -1.f, 1.f);
	int32 NumSteps = 1;

	if (bHasPrevAim && Dot < 0.99999f)
	{
		const float SweepAngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
		NumSteps = FMath::Clamp(FMath::CeilToInt(SweepAngleDeg / FMath::Max(AnglePerSubstepDeg, 0.1f)), 1, MaxSubsteps);
	}

	const int32 StartIndex = (NumSteps > 1) ? 0 : 1; // i==0 is FromDir: skip when identical to CurrDir
	
	FCollisionQueryParams Params = MakeWeaponTraceParams();
	Params.bReturnPhysicalMaterial = true;

	bool bHit = false;
	float BestDistSq = TNumericLimits<float>::Max();
	for (int32 i = StartIndex; i <= NumSteps; ++i)
	{
		const float Alpha = static_cast<float>(i) / static_cast<float>(NumSteps);
		const FVector Dir = FMath::Lerp(FromDir, CurrDir, Alpha).GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			continue;
		}
		const FVector End = Start + Dir * TraceRange;

		FHitResult StepHit;
		const bool bStepBlocking = World->LineTraceSingleByChannel(StepHit, Start, End, COLLISION_WEAPON, Params);

		UAbilitySystemComponent* HitASC = bStepBlocking ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(StepHit.GetActor()) : nullptr;

		if (HitASC)
		{
			const float DistSq = FVector::DistSquared(Start, StepHit.ImpactPoint);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				OutResult = StepHit;
				bHit = true;
			}	
		}

#if ENABLE_DRAW_DEBUG
		if (bDrawDebugTrace)
		{
			const FVector LineEnd = bStepBlocking ? StepHit.ImpactPoint : End;

			// green = hit a damageable target, yellow = hit world geo, red = hit nothing
			const FColor LineColor = HitASC ? FColor::Green : (bStepBlocking ? FColor::Yellow : FColor::Red);
			DrawDebugLine(World, Start, LineEnd, LineColor, false, 0.f, 0, 1.f);
			if (bStepBlocking)
			{
				DrawDebugPoint(World, StepHit.ImpactPoint, 8.f, LineColor, false, 0.f);
			}
		}
#endif
	}
	
	PrevAimDir = CurrDir;
	bHasPrevAim = true;
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
