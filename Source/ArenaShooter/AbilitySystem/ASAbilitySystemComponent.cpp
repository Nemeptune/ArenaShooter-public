// Fill out your copyright notice in the Description page of Project Settings.


#include "ASAbilitySystemComponent.h"

#include "Abilities/ASGameplayAbility.h"
#include "AbilitySystemLog.h"
#include "Net/UnrealNetwork.h"
#include "UniversalObjectLocators/AnimInstanceLocatorFragment.h"

static TAutoConsoleVariable<float> CVarReplayMontageErrorThreshold(
	TEXT("AS.replay.MontageErrorThreshold"),
	0.5f,
	TEXT("Tolerance level for when montage playback position correction occurs in replays")
);

void UASAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UASAbilitySystemComponent, RepAnimMontageInfoForMeshes);
}

void UASAbilitySystemComponent::AbilityInputPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}
	
	ABILITYLIST_SCOPE_LOCK();

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			if (!Spec.IsActive())
			{
				//TODO Use Activation policy
				TryActivateAbility(Spec.Handle);
			}
			else
			{
				if (const UGameplayAbility* Instance = Spec.GetPrimaryInstance())
				{
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
				}
			}
		}
	}
}

void UASAbilitySystemComponent::AbilityInputReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}
	
	ABILITYLIST_SCOPE_LOCK();

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			//TODO Use Activation policy
			if (const UGameplayAbility* Instance = Spec.GetPrimaryInstance())
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}
	}
}

bool UASAbilitySystemComponent::TryConsumeClientReplicatedTargetData(FGameplayAbilitySpecHandle AbilityHandle, FPredictionKey AbilityOriginalPredictionKey)
{
	TSharedPtr<FAbilityReplicatedDataCache> CachedData = AbilityTargetDataMap.Find(FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, AbilityOriginalPredictionKey));
	if (CachedData.IsValid())
	{
		const bool bConsumed = CachedData->TargetData.Num() > 0;
		CachedData->TargetData.Clear();
		CachedData->bTargetConfirmed = false;
		CachedData->bTargetCancelled = false;
		return bConsumed;
	}
	
	return false;
}

FGameplayEffectSpecHandle UASAbilitySystemComponent::MakeOutgoingSpecWithHitResult(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level, const FHitResult& HitResult, const UObject* SourceObject) const
{
	FGameplayEffectContextHandle EffectContext = MakeEffectContext();
	EffectContext.AddHitResult(HitResult);

	if (SourceObject)
	{
		EffectContext.AddSourceObject(SourceObject);
	}

	return MakeOutgoingSpec(GameplayEffectClass, Level, EffectContext);
}

float UASAbilitySystemComponent::PlayMontageForMesh(UGameplayAbility* InAnimatingAbility, class USkeletalMeshComponent* InMesh, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* NewAnimMontage, float InPlayRate, FName StartSectionName, bool bReplicateMontage)
{
	UASGameplayAbility* InAbility = Cast<UASGameplayAbility>(InAnimatingAbility);

	float Duration = -1.0f;

	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	if (AnimInstance && NewAnimMontage)
	{
		Duration = AnimInstance->Montage_Play(NewAnimMontage, InPlayRate);
		if (Duration > 0.0f)
		{
			FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);

			if (AnimMontageInfo.LocalMontageInfo.AnimatingAbility.IsValid() && AnimMontageInfo.LocalMontageInfo.AnimatingAbility != InAnimatingAbility)
			{
				// The ability that was previously animating will have already gotten the 'interrupted' callback.
				// It may be a good idea to make this a global policy and 'cancel' the ability.
				// 
				// For now, we expect it to end itself when this happens.
			}

			if (NewAnimMontage->HasRootMotion() && AnimInstance->GetOwningActor())
			{
				UE_LOG(LogRootMotion, Log, TEXT("UAbilitySystemComponent::PlayMontage %s, Role: %s")
					, *GetNameSafe(NewAnimMontage)
					, *UEnum::GetValueAsString(TEXT("Engine.ENetRole"), AnimInstance->GetOwningActor()->GetLocalRole())
					);
			}

			AnimMontageInfo.LocalMontageInfo.AnimMontage = NewAnimMontage;
			AnimMontageInfo.LocalMontageInfo.PlayInstanceId = (AnimMontageInfo.LocalMontageInfo.PlayInstanceId < UINT8_MAX ? AnimMontageInfo.LocalMontageInfo.PlayInstanceId + 1 : 0); // NEW
			AnimMontageInfo.LocalMontageInfo.AnimatingAbility = InAnimatingAbility;

			if (InAbility)
			{
				InAbility->SetCurrentMontageForMesh(InMesh, NewAnimMontage);
			}
			
			// Start at a given Section.
			if (StartSectionName != NAME_None)
			{
				AnimInstance->Montage_JumpToSection(StartSectionName, NewAnimMontage);
			}

			//Replicate to non owners
			if (IsOwnerActorAuthoritative())
			{
				// Those are static parameters, they are only set when the montage is played. They are not changed after that.
				FGameplayAbilityRepAnimMontageForMesh& AbilityRepMontageInfo = GetGameplayAbilityRepAnimMontageForMesh(InMesh);
				AbilityRepMontageInfo.RepMontageInfo.Animation = NewAnimMontage;

				// Update parameters that change during Montage lifetime
				AnimMontage_UpdateReplicatedDataForMesh(InMesh);

				// Force net update on our avatar actor
				if (AbilityActorInfo->AvatarActor != nullptr)
				{
					AbilityActorInfo->AvatarActor->ForceNetUpdate();
				}
			}
			else
			{
				// If this prediction key is rejected, we need to end the preview
				FPredictionKey PredictionKey = GetPredictionKeyForNewAction();
				if (PredictionKey.IsValidKey())
				{
					PredictionKey.NewRejectedDelegate().BindUObject(this, &UASAbilitySystemComponent::OnPredictiveMontageRejectedForMesh, InMesh, NewAnimMontage);
				}
			}
		}
	}
	if (!InMesh) { UE_LOG(LogTemp, Warning, TEXT("PlayMontageForMesh: InMesh is null")); return -1.f; }

	/*UE_LOG(LogTemp, Log, TEXT("Mesh: %s, Owner: %s, Avatar: %s, Visible: %d, TickEnabled: %d, PauseAnims: %d, GlobalRate: %.2f"),
		*GetNameSafe(InMesh),
		*GetNameSafe(InMesh->GetOwner()),
		*GetNameSafe(AbilityActorInfo ? AbilityActorInfo->AvatarActor.Get() : nullptr),
		InMesh->IsVisible(), InMesh->IsComponentTickEnabled(), InMesh->bPauseAnims, InMesh->GlobalAnimRateScale);
	
	UE_LOG(LogTemp, Log, TEXT("AnimInstance: %s, Montage: %s, InPlayRate: %.2f"),
		*GetNameSafe(AnimInstance), *GetNameSafe(NewAnimMontage), InPlayRate);*/
	
	return Duration;
}

float UASAbilitySystemComponent::PlayMontageSimulatedForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* NewAnimMontage, float InPlayRate, FName StartSectionName)
{
	float Duration = -1.0f;
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	if (AnimInstance && NewAnimMontage)
	{
		Duration = AnimInstance->Montage_Play(NewAnimMontage, InPlayRate);
		if (Duration > 0.0f)
		{
			FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);
			AnimMontageInfo.LocalMontageInfo.AnimMontage = NewAnimMontage;
		}
	}
	
	return Duration;
}

void UASAbilitySystemComponent::CurrentMontageStopForMesh(USkeletalMeshComponent* InMesh, float OverrideBlendOutTime)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);
	UAnimMontage* MontageToStop = AnimMontageInfo.LocalMontageInfo.AnimMontage;
	bool ShouldStopMontage = AnimInstance && MontageToStop && !AnimInstance->Montage_GetIsStopped(MontageToStop);

	if (ShouldStopMontage)
	{
		const float BlendOutTime = (OverrideBlendOutTime >= 0.0f ? OverrideBlendOutTime : MontageToStop->BlendOut.GetBlendTime());

		AnimInstance->Montage_Stop(BlendOutTime, MontageToStop);

		if (IsOwnerActorAuthoritative())
		{
			AnimMontage_UpdateReplicatedDataForMesh(InMesh);
		}
	}
}

UGameplayAbility* UASAbilitySystemComponent::GetAnimatingAbilityFromAnyMesh()
{
	// Only one ability can be animating for all meshes
	for (FGameplayAbilityLocalAnimMontageForMesh& GameplayAbilityLocalAnimMontageForMesh : LocalAnimMontageInfoForMeshes)
	{
		if (GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimatingAbility.IsValid())
		{
			return GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimatingAbility.Get();
		}
	}

	return nullptr;
}

TArray<UAnimMontage*> UASAbilitySystemComponent::GetCurrentMontages() const
{
	TArray<UAnimMontage*> Montages;

	for (FGameplayAbilityLocalAnimMontageForMesh GameplayAbilityLocalAnimMontageForMesh : LocalAnimMontageInfoForMeshes)
	{
		UAnimInstance* AnimInstance = IsValid(GameplayAbilityLocalAnimMontageForMesh.Mesh)
			&& GameplayAbilityLocalAnimMontageForMesh.Mesh->GetOwner() == AbilityActorInfo->AvatarActor ? GameplayAbilityLocalAnimMontageForMesh.Mesh->GetAnimInstance() : nullptr;
		if (GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimMontage && AnimInstance
			&& AnimInstance->Montage_IsActive(GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimMontage))
		{
			Montages.Add(GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimMontage);
		}
	}
	
	return Montages;
}

UAnimMontage* UASAbilitySystemComponent::GetCurrentMontageForMesh(USkeletalMeshComponent* InMesh)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);

	if (AnimMontageInfo.LocalMontageInfo.AnimMontage && AnimInstance
		&& AnimInstance->Montage_IsActive(AnimMontageInfo.LocalMontageInfo.AnimMontage))
	{
		return AnimMontageInfo.LocalMontageInfo.AnimMontage;
	}

	return nullptr;
}

FGameplayAbilityLocalAnimMontageForMesh& UASAbilitySystemComponent::GetLocalAnimMontageInfoForMesh(USkeletalMeshComponent* InMesh)
{
	for (FGameplayAbilityLocalAnimMontageForMesh& MontageInfo : LocalAnimMontageInfoForMeshes)
	{
		if (MontageInfo.Mesh == InMesh)
		{
			return MontageInfo;
		}
	}

	FGameplayAbilityLocalAnimMontageForMesh MontageInfo = FGameplayAbilityLocalAnimMontageForMesh(InMesh);
	LocalAnimMontageInfoForMeshes.Add(MontageInfo);
	return LocalAnimMontageInfoForMeshes.Last();
}

FGameplayAbilityRepAnimMontageForMesh& UASAbilitySystemComponent::GetGameplayAbilityRepAnimMontageForMesh(USkeletalMeshComponent* InMesh)
{
	for (FGameplayAbilityRepAnimMontageForMesh& RepMontageInfo : RepAnimMontageInfoForMeshes)
	{
		if (RepMontageInfo.Mesh == InMesh)
		{
			return RepMontageInfo;
		}
	}

	FGameplayAbilityRepAnimMontageForMesh RepMontageInfo = FGameplayAbilityRepAnimMontageForMesh(InMesh);
	RepAnimMontageInfoForMeshes.Add(RepMontageInfo);
	return RepAnimMontageInfoForMeshes.Last();
}

void UASAbilitySystemComponent::OnPredictiveMontageRejectedForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* PredictiveMontage)
{
	static const float MONTAGE_PREDICTION_REJECT_FADETIME = 0.25f;

	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	if (AnimInstance && PredictiveMontage)
	{
		// If this montage is still playing: kill it
		if (AnimInstance->Montage_IsPlaying(PredictiveMontage))
		{
			AnimInstance->Montage_Stop(MONTAGE_PREDICTION_REJECT_FADETIME, PredictiveMontage);
		}
	}
}

void UASAbilitySystemComponent::AnimMontage_UpdateReplicatedDataForMesh(USkeletalMeshComponent* InMesh)
{
	check(IsOwnerActorAuthoritative());

	AnimMontage_UpdateReplicatedDataForMesh(GetGameplayAbilityRepAnimMontageForMesh(InMesh));
}

void UASAbilitySystemComponent::AnimMontage_UpdateReplicatedDataForMesh(FGameplayAbilityRepAnimMontageForMesh& OutRepAnimMontageInfo)
{
	UAnimInstance* AnimInstance = IsValid(OutRepAnimMontageInfo.Mesh) && OutRepAnimMontageInfo.Mesh->GetOwner() == AbilityActorInfo->AvatarActor ?
		OutRepAnimMontageInfo.Mesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(OutRepAnimMontageInfo.Mesh);

	if (AnimInstance && AnimMontageInfo.LocalMontageInfo.AnimMontage)
	{
		OutRepAnimMontageInfo.RepMontageInfo.Animation = AnimMontageInfo.LocalMontageInfo.AnimMontage;
		OutRepAnimMontageInfo.RepMontageInfo.PlayInstanceId = AnimMontageInfo.LocalMontageInfo.PlayInstanceId; // NEW

		//Compressed Flags
		bool bIsStopped = AnimInstance->Montage_GetIsStopped(AnimMontageInfo.LocalMontageInfo.AnimMontage);

		if (!bIsStopped)
		{
			OutRepAnimMontageInfo.RepMontageInfo.PlayRate = AnimInstance->Montage_GetPlayRate(AnimMontageInfo.LocalMontageInfo.AnimMontage);
			OutRepAnimMontageInfo.RepMontageInfo.Position = AnimInstance->Montage_GetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage);
			OutRepAnimMontageInfo.RepMontageInfo.BlendTime = AnimInstance->Montage_GetBlendTime(AnimMontageInfo.LocalMontageInfo.AnimMontage);
		}

		if (OutRepAnimMontageInfo.RepMontageInfo.IsStopped != bIsStopped)
		{
			// Set this prior to calling UpdateShouldTick, so we can start ticking if we are playing a Montage
			OutRepAnimMontageInfo.RepMontageInfo.IsStopped = bIsStopped;

			// When we start or stop an animation, update the clients right away for the Avatar Actor
			if (AbilityActorInfo->AvatarActor != nullptr)
			{
				AbilityActorInfo->AvatarActor->ForceNetUpdate();
			}

			// when this changes, we should update whether or not we should be ticking
			UpdateShouldTick();
		}

		// Replicate NextSectionID to keep it in sync.
		// We actually replicate NextSectionID+1 on a BYTE to put INDEX_NONE in there.
		int32 CurrentSectionID = AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionIndexFromPosition(OutRepAnimMontageInfo.RepMontageInfo.Position);
		if (CurrentSectionID != INDEX_NONE)
		{
			int32 NextSectionID = AnimInstance->Montage_GetNextSectionID(AnimMontageInfo.LocalMontageInfo.AnimMontage, CurrentSectionID);
			if (NextSectionID >= (256 - 1))
			{
				ABILITY_LOG(Error, TEXT("AnimMontage_UpdateReplicatedData. NextSectionID = %d.  RepAnimMontageInfo.Position: %.2f, CurrentSectionID: %d. LocalAnimMontageInfo.AnimMontage %s"),
					NextSectionID, OutRepAnimMontageInfo.RepMontageInfo.Position, CurrentSectionID, *GetNameSafe(AnimMontageInfo.LocalMontageInfo.AnimMontage));
				ensure(NextSectionID < (256 - 1));
			}
			OutRepAnimMontageInfo.RepMontageInfo.NextSectionID = uint8(NextSectionID + 1);
		}
		else
		{
			OutRepAnimMontageInfo.RepMontageInfo.NextSectionID = 0;
		}
	}
}

void UASAbilitySystemComponent::OnRep_ReplicatedAnimMontageForMesh()
{
	for (FGameplayAbilityRepAnimMontageForMesh& NewRepAnimMontageInfoForMesh : RepAnimMontageInfoForMeshes)
	{
		FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(NewRepAnimMontageInfoForMesh.Mesh);

		UWorld* World = GetWorld();

		if (NewRepAnimMontageInfoForMesh.RepMontageInfo.bSkipPlayRate)
		{
			NewRepAnimMontageInfoForMesh.RepMontageInfo.PlayRate = 1.f;
		}

		const bool bIsPlayingReplay = World->IsPlayingReplay();

		const float MONTAGE_REP_POS_ERR_THRESH = bIsPlayingReplay ? CVarReplayMontageErrorThreshold.GetValueOnGameThread() : 0.1f;

		UAnimInstance* AnimInstance = IsValid(NewRepAnimMontageInfoForMesh.Mesh) && NewRepAnimMontageInfoForMesh.Mesh->GetOwner()
			== AbilityActorInfo->AvatarActor ? NewRepAnimMontageInfoForMesh.Mesh->GetAnimInstance() : nullptr;
		if(AnimInstance == nullptr || !IsReadyForReplicatedMontageForMesh())
		{
			// We can't handle this yet
			bPendingMontageRep = true;
			return;
		}
		// TODO there probably needs to be bPendingMontageRepForMesh
		bPendingMontageRep = false;

		if (!AbilityActorInfo->IsLocallyControlled())
		{
			static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("net.Montage.Debug"));
			bool DebugMontage = (CVar && CVar->GetValueOnGameThread() == 1);
			if (DebugMontage)
			{
				ABILITY_LOG(Warning, TEXT("\n\nOnRep_ReplicatedAnimMontageForMesh, %s"), *GetNameSafe(this));
				ABILITY_LOG(Warning, TEXT("\tAnimMontage: %s \n\tPlayRate: %f\n\tPosition: %f\n\tBlendTime: %f\n\tNextSectionID: %d\n\tIsStopped: %d"),
					*GetNameSafe(NewRepAnimMontageInfoForMesh.RepMontageInfo.GetAnimMontage()),
					NewRepAnimMontageInfoForMesh.RepMontageInfo.PlayRate,
					NewRepAnimMontageInfoForMesh.RepMontageInfo.Position,
					NewRepAnimMontageInfoForMesh.RepMontageInfo.BlendTime,
					NewRepAnimMontageInfoForMesh.RepMontageInfo.NextSectionID,
					NewRepAnimMontageInfoForMesh.RepMontageInfo.IsStopped);
				ABILITY_LOG(Warning, TEXT("\tLocalAnimMontageInfo.AnimMontage: %s\n\tPosition: %f"),
					*GetNameSafe(AnimMontageInfo.LocalMontageInfo.AnimMontage), AnimInstance->Montage_GetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage));
			}

			if (NewRepAnimMontageInfoForMesh.RepMontageInfo.GetAnimMontage())
			{
				// New Montage to play
				if (AnimMontageInfo.LocalMontageInfo.AnimMontage != NewRepAnimMontageInfoForMesh.RepMontageInfo.GetAnimMontage() || AnimMontageInfo.LocalMontageInfo.PlayInstanceId != NewRepAnimMontageInfoForMesh.RepMontageInfo.PlayInstanceId)
				{
					AnimMontageInfo.LocalMontageInfo.PlayInstanceId = NewRepAnimMontageInfoForMesh.RepMontageInfo.PlayInstanceId;
					PlayMontageSimulatedForMesh(NewRepAnimMontageInfoForMesh.Mesh, NewRepAnimMontageInfoForMesh.RepMontageInfo.GetAnimMontage(), NewRepAnimMontageInfoForMesh.RepMontageInfo.PlayRate);
				}

				if (AnimMontageInfo.LocalMontageInfo.AnimMontage == nullptr)
				{
					ABILITY_LOG(Warning, TEXT("OnRep_ReplicatedAnimMontageForMesh: PlayMontageSimulated failed. Name: %s, AnimMontage: %s"), *GetNameSafe(this), *GetNameSafe(NewRepAnimMontageInfoForMesh.RepMontageInfo.GetAnimMontage()));
					return;
				}

				// Play Rate has changed
				if (AnimInstance->Montage_GetPlayRate(AnimMontageInfo.LocalMontageInfo.AnimMontage) != NewRepAnimMontageInfoForMesh.RepMontageInfo.PlayRate)
				{
					AnimInstance->Montage_SetPlayRate(AnimMontageInfo.LocalMontageInfo.AnimMontage, NewRepAnimMontageInfoForMesh.RepMontageInfo.PlayRate);
				}

				//Compressed Flags
				const bool bIsStopped = AnimInstance->Montage_GetIsStopped(AnimMontageInfo.LocalMontageInfo.AnimMontage);
				const bool bReplicatedIsStopped = bool(NewRepAnimMontageInfoForMesh.RepMontageInfo.IsStopped);

				// Process stopping first, so we don't change sections and cause blending to pop.
				if (bReplicatedIsStopped)
				{
					if (!bIsStopped)
					{
						CurrentMontageStopForMesh(NewRepAnimMontageInfoForMesh.Mesh, NewRepAnimMontageInfoForMesh.RepMontageInfo.BlendTime);
					}
				}
				else if (!NewRepAnimMontageInfoForMesh.RepMontageInfo.SkipPositionCorrection)
				{
					const int32 RepSectionID = AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionIndexFromPosition(NewRepAnimMontageInfoForMesh.RepMontageInfo.Position);
					const int32 RepNextSectionID = int32(NewRepAnimMontageInfoForMesh.RepMontageInfo.NextSectionID) - 1;

					// And NextSectionID for the replicated SectionID
					if (RepSectionID != INDEX_NONE)
					{
						const int32 NextSectionID = AnimInstance->Montage_GetNextSectionID(AnimMontageInfo.LocalMontageInfo.AnimMontage, RepSectionID);

						// If NextSectionID is different from the replicated one, then set it
						if (NextSectionID != RepNextSectionID)
						{
							AnimInstance->Montage_SetNextSection(AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionName(RepSectionID), AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionName(RepNextSectionID), AnimMontageInfo.LocalMontageInfo.AnimMontage);
						}

						// Make sure we haven't received that update too late and the client hasn't already jumped to another section.
						const int32 CurrentSectionID = AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionIndexFromPosition(AnimInstance->Montage_GetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage));
						if ((CurrentSectionID != RepSectionID) && (CurrentSectionID != RepNextSectionID))
						{
							// Client is in a wrong section, teleport him into the beginning of the right section
							const float SectionStartTime = AnimMontageInfo.LocalMontageInfo.AnimMontage->GetAnimCompositeSection(RepSectionID).GetTime();
							AnimInstance->Montage_SetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage, SectionStartTime);
						}
					}

					// Update Position. If error is too great, jump to replicated position.
					const float CurrentPosition = AnimInstance->Montage_GetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage);
					const int32 CurrentSectionID = AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionIndexFromPosition(CurrentPosition);
					const float DeltaPosition = NewRepAnimMontageInfoForMesh.RepMontageInfo.Position - CurrentPosition;

					// Only check threshold if we are located in the same section. Different sections require a bit more work as we could be jumping around the timeline.
					// And therefore DeltaPosition is not as trivial to determine.
					if ((CurrentSectionID == RepSectionID) && (FMath::Abs(DeltaPosition) > MONTAGE_REP_POS_ERR_THRESH) && (NewRepAnimMontageInfoForMesh.RepMontageInfo.IsStopped == 0))
					{
						// Fast-forward to server position and trigger notifies
						if (FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(NewRepAnimMontageInfoForMesh.RepMontageInfo.GetAnimMontage()))
						{
							// Skip triggering notifies if we're going backwards in time, we've already triggered them.
							const float DeltaTime = !FMath::IsNearlyZero(NewRepAnimMontageInfoForMesh.RepMontageInfo.PlayRate) ? (DeltaPosition / NewRepAnimMontageInfoForMesh.RepMontageInfo.PlayRate) : 0.0f;
							if (DeltaTime > 0.0f)
							{
								MontageInstance->UpdateWeight(DeltaTime);
								MontageInstance->HandleEvents(CurrentPosition, NewRepAnimMontageInfoForMesh.RepMontageInfo.Position, nullptr);
								AnimInstance->TriggerAnimNotifies(DeltaTime);
							}
						}

						AnimInstance->Montage_SetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage, NewRepAnimMontageInfoForMesh.RepMontageInfo.Position);
					}
				}
			}
		}
	}
}

bool UASAbilitySystemComponent::IsReadyForReplicatedMontageForMesh()
{
	/** Children may want to override this for additional checks (e.g, "has skin been applied") */
	return true;
}
