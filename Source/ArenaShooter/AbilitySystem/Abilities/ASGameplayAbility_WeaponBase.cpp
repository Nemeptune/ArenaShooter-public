// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ASGameplayAbility_WeaponBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ArenaShooter.h"
#include "Weapon/ASWeaponInstance.h"

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
	check(CurrentActorInfo);

	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	FScopedPredictionWindow ScopedPredictionWindow(MyAbilityComponent , CurrentActivationInfo.GetActivationPredictionKey());

	TArray<FHitResult> FoundHits;
	PerformLocalTargeting( FoundHits);

	//Fill target data from the hit results
	FGameplayAbilityTargetDataHandle TargetData;
	for (const FHitResult& FoundHit : FoundHits)
	{
		FGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new FGameplayAbilityTargetData_SingleTargetHit();
		NewTargetData->HitResult = FoundHit;
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

		if (CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
		{
			NotifyWeaponFired();
			HandleTargetDataOnAuthority(LocalTargetDataHandle);
			ExecuteFireCue(LocalTargetDataHandle);
			OnRangedWeaponTargetDataReady(LocalTargetDataHandle);
		}

		MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}
}

void UASGameplayAbility_WeaponBase::PerformLocalTargeting(TArray<FHitResult>& OutHits)
{
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