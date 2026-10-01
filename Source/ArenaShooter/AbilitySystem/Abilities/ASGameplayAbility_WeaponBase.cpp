// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ASGameplayAbility_WeaponBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ArenaShooter.h"
#include "System/ASLogChannels.h"
#include "System/ASProfiling.h"
#include "Weapon/ASLagCompensationSubsystem.h"
#include "Weapon/ASWeaponInstance.h"
#include "Weapon/ASShotValidation.h"

void UASGameplayAbility_WeaponBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	OnTargetDataReadyCallbackDelegateHandle = MyAbilityComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UASGameplayAbility_WeaponBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		if (ScopeLockCount > 0)
		{
			WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility, Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
			return;
		}

		UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
		check(MyAbilityComponent);

		// When ability ends, consume target data and remove delegate
		MyAbilityComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(OnTargetDataReadyCallbackDelegateHandle);
		MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}
}

void UASGameplayAbility_WeaponBase::StartRangedWeaponTargeting()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASGameplayAbility_WeaponBase::StartRangedWeaponTargeting);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_WeaponFire);
	
	check(CurrentActorInfo);

	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	FScopedPredictionWindow ScopedPredictionWindow(MyAbilityComponent , CurrentActivationInfo.GetActivationPredictionKey());

	TArray<FHitResult> FoundHits;
	PerformLocalTargeting( FoundHits);

	// The moment this screen showed, so the server can rewind its own trace to what the shot was aimed at.
	const UASLagCompensationSubsystem* LagCompensation = GetWorld()->GetSubsystem<UASLagCompensationSubsystem>();
	const double ViewServerTime = LagCompensation ? LagCompensation->GetShownServerTime() : 0.;

	//Fill target data from the hit results
	FGameplayAbilityTargetDataHandle TargetData;
	for (const FHitResult& FoundHit : FoundHits)
	{
		FASGameplayAbilityTargetData_ShotHit* NewTargetData = new FASGameplayAbilityTargetData_ShotHit();
		NewTargetData->HitResult = FoundHit;
		NewTargetData->ViewServerTime = ViewServerTime;
		TargetData.Add(NewTargetData);
	}
	
	// Process the target data immediately
	OnTargetDataReadyCallback(TargetData, FGameplayTag());
}

void UASGameplayAbility_WeaponBase::HandleTargetDataOnAuthority(const FGameplayAbilityTargetDataHandle& TargetData)
{
	if (!CurrentActorInfo->IsNetAuthority())
	{
		return;
	}
	
	TArray<FHitResult> Hits;
	Hits.Reserve(TargetData.Num());
	
	for (int32 i = 0; i < TargetData.Num(); ++i)
	{
		if (const FHitResult* Hit = TargetData.Get(i)->GetHitResult())
		{
			Hits.Add(*Hit);
		}
	}

	ApplyDamageEffectToTargets(Hits);
}

void UASGameplayAbility_WeaponBase::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASGameplayAbility_WeaponBase::OnTargetDataReadyCallback);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_WeaponFire);
	
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	if (const FGameplayAbilitySpec* AbilitySpec = MyAbilityComponent->FindAbilitySpecFromHandle(CurrentSpecHandle))
	{
		FScopedPredictionWindow ScopedPrediction(MyAbilityComponent);

		// Take ownership of the target data to make sure no callbacks into game code invalidate it out from under us
		FGameplayAbilityTargetDataHandle LocalTargetDataHandle(MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

		const bool bShouldNotifyServer = CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority();
		if (bShouldNotifyServer)
		{
			MyAbilityComponent->CallServerSetReplicatedTargetData(CurrentSpecHandle,CurrentActivationInfo.GetActivationPredictionKey(), LocalTargetDataHandle, ApplicationTag, MyAbilityComponent->ScopedPredictionKey);
		}

		// A remote client's hits are only its claim. The server traces the same rays itself, so nothing the
		// client says it hit reaches damage, the cue or Blueprint.
		const bool bIsRemoteClientShot = CurrentActorInfo->IsNetAuthority() && !CurrentActorInfo->IsLocallyControlled();
		const bool bShotStands = !bIsRemoteClientShot || RetraceClientShot(LocalTargetDataHandle);

		if (bShotStands && CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
		{
			CSV_CUSTOM_STAT(ArenaShooter, Shots, 1, ECsvCustomStatOp::Accumulate);
			NotifyWeaponFired();
			HandleTargetDataOnAuthority(LocalTargetDataHandle);
			ExecuteFireCue(LocalTargetDataHandle);
			OnRangedWeaponTargetDataReady(LocalTargetDataHandle);
		}

		MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}
}

bool UASGameplayAbility_WeaponBase::RetraceClientShot(FGameplayAbilityTargetDataHandle& InOutTargetData) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASGameplayAbility_WeaponBase::RetraceClientShot);

	const UASWeaponInstance* Weapon = GetSourceWeapon();
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	const UASLagCompensationSubsystem* LagCompensation = GetWorld()->GetSubsystem<UASLagCompensationSubsystem>();
	if (!Weapon || !Avatar || !LagCompensation)
	{
		return false;
	}

	// Activation checked the refire time, but one activation can carry any number of shots.
	if (!Weapon->CanFire())
	{
		UE_LOG(LogAS_Weapon, Warning, TEXT("%s: dropped a shot from %s that came sooner than the weapon refires"), *GetName(), *GetNameSafe(Avatar));
		return false;
	}

	// The server keeps each bullet only as a direction. The first bullet's origin and view time speak for the whole shot.
	const int32 NumBullets = InOutTargetData.Num();
	FVector Origin = FVector::ZeroVector;
	double ViewServerTime = 0.;
	TArray<FVector> Directions;
	Directions.Reserve(NumBullets);
	for (int32 Index = 0; Index < NumBullets; ++Index)
	{
		const FGameplayAbilityTargetData* Data = InOutTargetData.Get(Index);
		const FHitResult* ClaimedHit = Data ? Data->GetHitResult() : nullptr;
		Directions.Add(ClaimedHit ? (ClaimedHit->TraceEnd - ClaimedHit->TraceStart).GetSafeNormal() : FVector::ZeroVector);

		if (Index == 0 && ClaimedHit)
		{
			Origin = ClaimedHit->TraceStart;
			if (Data->GetScriptStruct()->IsChildOf(FASGameplayAbilityTargetData_ShotHit::StaticStruct()))
			{
				ViewServerTime = static_cast<const FASGameplayAbilityTargetData_ShotHit*>(Data)->ViewServerTime;
			}
		}
	}

	const EASShotVerdict Verdict = ASShotValidation::CheckBullets(Directions, Weapon->GetBulletsPerShot(), Weapon->GetSpreadHalfAngleDeg());
	if (Verdict != EASShotVerdict::Valid)
	{
		UE_LOG(LogAS_Weapon, Warning, TEXT("%s: dropped a shot from %s with %s (%d bullets, the weapon fires %d)"),
			*GetName(), *GetNameSafe(Avatar), LexToString(Verdict), NumBullets, Weapon->GetBulletsPerShot());
		return false;
	}
	if (NumBullets == 0)
	{
		return true; // nothing to hit
	}
	
	if (!ClampClientShotOrigin(Avatar, Origin))
	{
		return false;
	}

	TArray<FASShotRay> Rays;
	for (const FVector& Direction : Directions)
	{
		Rays.Add({ Origin, Origin + Direction * TraceRange });
	}

	TArray<FHitResult> Hits;
	Hits.SetNum(Rays.Num());
	LagCompensation->LineTraceRewound(Rays, COLLISION_WEAPON, MakeWeaponTraceParams(), Avatar, Hits, ViewServerTime);

	FGameplayAbilityTargetDataHandle ServerTargetData;
	for (int32 Index = 0; Index < Hits.Num(); ++Index)
	{
		// Hit registration misses: the client drew a hit the rewound trace doesn't find. A few are latency, many are a tuning problem.
		const AActor* ClaimedActor = InOutTargetData.Get(Index)->GetHitResult()->GetActor();
		if (ClaimedActor && ClaimedActor != Hits[Index].GetActor())
		{
			CSV_CUSTOM_STAT(ArenaShooter, HitRegMismatches, 1, ECsvCustomStatOp::Accumulate);
			UE_LOG(LogAS_Weapon, Verbose, TEXT("%s: %s hit %s on its screen, the server's trace hit %s"),
				*GetName(), *GetNameSafe(Avatar), *GetNameSafe(ClaimedActor), *GetNameSafe(Hits[Index].GetActor()));
		}
		ServerTargetData.Add(new FGameplayAbilityTargetData_SingleTargetHit(Hits[Index]));
	}

	InOutTargetData = MoveTemp(ServerTargetData);
	return true;
}

void UASGameplayAbility_WeaponBase::PerformLocalTargeting(TArray<FHitResult>& OutHits)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASGameplayAbility_WeaponBase::PerformLocalTargeting);
	
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetWeaponViewpoint(ViewLocation, ViewRotation))
	{
		return;
	}

	int32 NumBullets = 1;
	float SpreadRad = 0.0f;

	if (const UASWeaponInstance* Weapon = GetSourceWeapon())
	{
		NumBullets = Weapon->GetBulletsPerShot();
		SpreadRad = FMath::DegreesToRadians(Weapon->GetSpreadHalfAngleDeg());
	}
	
	const FVector BaseDir = ViewRotation.Vector();
	const FCollisionQueryParams Params = MakeWeaponTraceParams();

	for (int32 i = 0; i < NumBullets; ++i)
	{
		const FVector Dir = (SpreadRad > 0.f) ? FMath::VRandCone(BaseDir, SpreadRad) : BaseDir;
		const FVector End = ViewLocation + Dir * TraceRange;
		
		FHitResult Hit;
		GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, End, COLLISION_WEAPON, Params);
		OutHits.Add(Hit);
	}
}