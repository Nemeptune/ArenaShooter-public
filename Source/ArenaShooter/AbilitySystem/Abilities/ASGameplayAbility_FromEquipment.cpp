// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameplayAbility_FromEquipment.h"

#include "AbilitySystemComponent.h"
#include "ArenaShooter.h"
#include "System/ASGameplayTags.h"
#include "System/ASLogChannels.h"
#include "Physics/ASPhysicalMaterial.h"
#include "AbilitySystem/ASGameplayEffectContext.h"
#include "System/ASProfiling.h"
#include "Weapon/ASWeaponInstance.h"

UASGameplayAbility_FromEquipment::UASGameplayAbility_FromEquipment()
{
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(FASGameplayTags::Ability_Weapon);   // CancelAbilities filters on for swap-cancel
	SetAssetTags(Tags);

	ActivationBlockedTags.AddTag(FASGameplayTags::Ability_Weapon_IsChanging);
}

bool UASGameplayAbility_FromEquipment::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UASWeaponInstance* Weapon = Cast<UASWeaponInstance>(GetSourceObject(Handle, ActorInfo));
	if (!Weapon || !Weapon->IsActive() || !Weapon->CanFire())
	{
		UE_LOG(LogAS_Weapon, VeryVerbose, TEXT("%s refused on %s: Weapon=%s Active=%d CanFire=%d"),
			*GetName(), ActorInfo->IsNetAuthority() ? TEXT("server") : TEXT("client"),
			*GetNameSafe(Weapon), Weapon && Weapon->IsActive(), Weapon && Weapon->CanFire());
		return false;
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UASGameplayAbility_FromEquipment::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UASWeaponInstance* Weapon = Cast<UASWeaponInstance>(GetSourceObject(Handle, ActorInfo));
	if (!Weapon || Weapon->GetAmmo() < Weapon->GetFireCost())
	{
		UE_LOG(LogAS, Verbose, TEXT("%s refused on %s: Ammo=%d Cost=%d"),
			*GetName(), ActorInfo->IsNetAuthority() ? TEXT("server") : TEXT("client"),
				Weapon ? Weapon->GetAmmo() : -1,  Weapon ? Weapon->GetFireCost() : -1);
		return false;
	}
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UASGameplayAbility_FromEquipment::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
	
	UASWeaponInstance* Weapon = Cast<UASWeaponInstance>(GetSourceObject(Handle, ActorInfo));
	if (!Weapon || !ActorInfo)
	{
		return;
	}
	
	const int32 Cost = Weapon->GetFireCost();
	
	if (ActorInfo->IsNetAuthority())
	{
		Weapon->ConsumeAmmo(Cost);
	}
	else if (ActorInfo->IsLocallyControlled())
	{
		Weapon->PredictSpendAmmo(Cost);
	}
}

UASWeaponInstance* UASGameplayAbility_FromEquipment::GetSourceWeapon() const
{
	return Cast<UASWeaponInstance>(GetCurrentSourceObject());
}

void UASGameplayAbility_FromEquipment::NotifyWeaponFired()
{
	if (UASWeaponInstance* Weapon = GetSourceWeapon())
	{
		Weapon->MarkFired();
	}
}

void UASGameplayAbility_FromEquipment::ExecuteFireCue(const FGameplayAbilityTargetDataHandle& TargetData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASGameplayAbility_FromEquipment::ExecuteFireCue);
	
	if (!FireCueTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	if (FGameplayEffectContext* Raw = Context.Get())
	{
		if (Raw->GetScriptStruct()->IsChildOf(FASGameplayEffectContext::StaticStruct()))
		{
			static_cast<FASGameplayEffectContext*>(Raw)->TargetData = TargetData;
		}
	}

	FGameplayCueParameters CueParameters(Context);
	CueParameters.SourceObject = GetSourceWeapon();   // the cue notifies cast this back to the instance
	CueParameters.Instigator = GetAvatarActorFromActorInfo();

	UE_LOG(LogAS, Verbose, TEXT("FireCue SEND: tag=%s weapon=%s avatar=%s authority=%d locallyControlled=%d targets=%d"),
		*FireCueTag.ToString(),
		*CueParameters.SourceObject->GetName(),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		(int32)CurrentActorInfo->IsNetAuthority(),
		(int32)CurrentActorInfo->IsLocallyControlled(),
		TargetData.Num());
	
	if (CurrentActorInfo->IsNetAuthority())
	{
		// On the server this is the unreliable multicast every client receives.
		CSV_CUSTOM_STAT(ArenaShooter, FireCuesSent, 1, ECsvCustomStatOp::Accumulate);
	}
	
	ASC->ExecuteGameplayCue(FireCueTag, CueParameters);
}

bool UASGameplayAbility_FromEquipment::GetWeaponViewpoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return false;
	}
	
	Avatar->GetActorEyesViewPoint(OutLocation, OutRotation);
	return true;
}

FCollisionQueryParams UASGameplayAbility_FromEquipment::MakeWeaponTraceParams() const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponTrace), true, Avatar);
	Params.AddIgnoredActor(Avatar);
	Params.bReturnPhysicalMaterial = true;

	return Params;
}

bool UASGameplayAbility_FromEquipment::ApplyEffectToTargetFromHit(TSubclassOf<UGameplayEffect> EffectClass, const FHitResult& Hit) const
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hit.GetActor());
	if (!SourceASC || !TargetASC || !EffectClass)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	Context.AddInstigator(Avatar, Avatar);
	Context.AddHitResult(Hit);

	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), Context);
	if (!Spec.IsValid())
	{
		return false;
	}

	return SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC).WasSuccessfullyApplied();
}

int32 UASGameplayAbility_FromEquipment::ApplyDamageEffectToTargets(TArrayView<const FHitResult> Hits) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UASGameplayAbility_FromEquipment::ApplyDamageEffectToTargets);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_Damage);
	
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const UASWeaponInstance* Weapon = GetSourceWeapon();
	
	if (!SourceASC || !Weapon || !DamageEffectClass || Hits.IsEmpty())
	{
		return 0;
	}
	
	struct FAccumulated
	{
		FHitResult BestHit;
		float BestMultiplier = -1.f;
		float TotalDamage = 0.f;
		FGameplayTagContainer ZoneTags;
	};
	
	TMap<AActor*, FAccumulated> PerTarget;
	PerTarget.Reserve(Hits.Num());
	
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor)
		{
			continue;
		}
		
		const UPhysicalMaterial* PhysMat = Hit.PhysMaterial.Get();
		const float Multiplier = UASPhysicalMaterial::GetDamageMultiplier(PhysMat);
		
		FAccumulated& Acc = PerTarget.FindOrAdd(HitActor);
		Acc.TotalDamage += Weapon->GetBaseDamage() * Multiplier;
		
		if (Multiplier > Acc.BestMultiplier)
		{
			Acc.BestMultiplier = Multiplier;
			Acc.BestHit = Hit;
			
			Acc.ZoneTags.Reset();
			if (const UASPhysicalMaterial* TaggedMat = Cast<const UASPhysicalMaterial>(PhysMat))
			{
				Acc.ZoneTags = TaggedMat->Tags;
			}
		}
	}
	
	AActor* Avatar = GetAvatarActorFromActorInfo();
	int32 NumDamaged = 0;

	for (const TPair<AActor*, FAccumulated>& Pair : PerTarget)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pair.Key);
		if (!TargetASC || Pair.Value.TotalDamage <= 0.f)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddInstigator(Avatar, Avatar);
		Context.AddHitResult(Pair.Value.BestHit);

		const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), Context);
		if (!Spec.IsValid())
		{
			continue;
		}

		Spec.Data->AppendDynamicAssetTags(Pair.Value.ZoneTags);
		Spec.Data->SetSetByCallerMagnitude(FASGameplayTags::Data_Damage, Pair.Value.TotalDamage);

		if (SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC).WasSuccessfullyApplied())
		{
			++NumDamaged;
		}
	}
	
	return NumDamaged;
}
