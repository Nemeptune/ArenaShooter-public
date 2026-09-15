// Fill out your copyright notice in the Description page of Project Settings.


#include "ASAT_PlayMontageForMeshAndWait.h"

#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "ASLogChannels.h"
#include "GameFramework/Character.h"

UASAT_PlayMontageForMeshAndWait::UASAT_PlayMontageForMeshAndWait(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	Rate = 1.0f;
	bStopWhenAbilityEnds = true;
}

UASAT_PlayMontageForMeshAndWait* UASAT_PlayMontageForMeshAndWait::PlayMontageForMeshAndWait(UGameplayAbility* OwningAbility,
	FName TaskInstanceName, USkeletalMeshComponent* InMesh, UAnimMontage* MontageToPlay, float Rate, FName StartSection,
	bool bStopWhenAbilityEnds, float AnimRootMotionTranslationScale, bool bReplicateMontage, float OverrideBlendOutTimeForCancelAbility,
	float OverrideBlendOutTimeForStopWhenEndAbility)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);

	UASAT_PlayMontageForMeshAndWait* MyObj = NewAbilityTask<UASAT_PlayMontageForMeshAndWait>(OwningAbility, TaskInstanceName);
	MyObj->Mesh = InMesh;
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;
	MyObj->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	MyObj->bReplicateMontage = bReplicateMontage;
	MyObj->OverrideBlendOutTimeForCancelAbility = OverrideBlendOutTimeForCancelAbility;
	MyObj->OverrideBlendOutTimeForStopWhenEndAbility = OverrideBlendOutTimeForStopWhenEndAbility;

	return MyObj;
}

void UASAT_PlayMontageForMeshAndWait::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	const bool bPlayingThisMontage = (Montage == MontageToPlay) && Ability && Ability->GetCurrentMontage() == MontageToPlay;
	if (bPlayingThisMontage)
	{
		// Reset AnimRootMotionTranslationScale
		ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
		if (Character && (Character->GetLocalRole() == ROLE_Authority ||
							(Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
		{
			Character->SetAnimRootMotionTranslationScale(1.f);
		}
	}

	if (bPlayingThisMontage && bInterrupted)
	{
		if (UAbilitySystemComponent* ASC = GetTargetASC())
		{
			ASC->ClearAnimatingAbility(Ability);
		}
	}
	
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (bInterrupted)
		{
			OnInterrupted.Broadcast();
		}
		else
		{
			OnBlendOut.Broadcast();
		}
	}
}

void UASAT_PlayMontageForMeshAndWait::OnAbilityCancelled()
{
	// TODO: Merge this fix back to engine, it was calling the wrong callback
	if (StopPlayingMontage(OverrideBlendOutTimeForCancelAbility))
	{
		// Let the BP handle the interrupt as well
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast();
		}
	}
}

void UASAT_PlayMontageForMeshAndWait::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast();
		}
	}

	EndTask();
}

// Calling PlayMontageForMesh instead of PlayMontage
void UASAT_PlayMontageForMeshAndWait::Activate()
{
	if (Ability == nullptr)
	{
		return;
	}
	if (Mesh == nullptr)
	{
		UE_LOG(LogAS, Error, TEXT("%s invalid Mesh"), *FString(__FUNCTION__));
		return;
	}
	bool bPlayedMontage = false;
	if (UASAbilitySystemComponent* ASC = GetTargetASC())
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			if (ASC->PlayMontageForMesh(Ability, Mesh, Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, StartSection, bReplicateMontage) > 0.f)
			{
				// Playing a montage could potentially fire off a callback into game code which could kill this ability! Early out if we are  pending kill.
				if (ShouldBroadcastAbilityTaskDelegates() == false)
				{
					return;
				}

				InterruptedHandle = Ability->OnGameplayAbilityCancelled.AddUObject(this, &UASAT_PlayMontageForMeshAndWait::OnAbilityCancelled);

				BlendingOutDelegate.BindUObject(this, &UASAT_PlayMontageForMeshAndWait::OnMontageBlendingOut);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

				MontageEndedDelegate.BindUObject(this, &UASAT_PlayMontageForMeshAndWait::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

				ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
				if (Character && (Character->GetLocalRole() == ROLE_Authority ||
								  (Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
				{
					Character->SetAnimRootMotionTranslationScale(AnimRootMotionTranslationScale);
				}

				bPlayedMontage = true;
			}
		}
		else
		{
			UE_LOG(LogAS, Warning, TEXT("UASAbilityTask_PlayMontageForMeshAndWait call to PlayMontage failed!"))
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UASAbilityTask_PlayMontageForMeshAndWait called on invalid AbilitySystemComponent"));
	}

	if (!bPlayedMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("UASAbilityTask_PlayMontageForMeshAndWait called in Ability %s failed to play montage %s; Task Instance Name %s."), *Ability->GetName(), *GetNameSafe(MontageToPlay), *InstanceName.ToString());
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast();
		}
	}

	SetWaitingOnAvatar();
}

void UASAT_PlayMontageForMeshAndWait::ExternalCancel()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelled.Broadcast();
	}
	Super::ExternalCancel();
}

FString UASAT_PlayMontageForMeshAndWait::GetDebugString() const
{
	UAnimMontage* PlayingMontage = nullptr;
	if (Ability && Mesh)
	{
		UAnimInstance* AnimInstance = Mesh->GetAnimInstance();

		if (AnimInstance != nullptr)
		{
			PlayingMontage = AnimInstance->Montage_IsActive(MontageToPlay.Get()) ? MontageToPlay.Get() : AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(TEXT("PlayMontageAndWait. MontageToPlay: %s  (Currently Playing): %s"), *GetNameSafe(MontageToPlay), *GetNameSafe(PlayingMontage));
}

void UASAT_PlayMontageForMeshAndWait::OnDestroy(bool AbilityEnded)
{
	// Note: Clearing montage end delegate isn't necessary since its not a multicast and will be cleared when the next montage plays.
	// (If we are destroyed, it will detect this and not do anything)

	// This delegate, however, should be cleared as it is a multicast
	if (Ability)
	{
		Ability->OnGameplayAbilityCancelled.Remove(InterruptedHandle);
		if (AbilityEnded && bStopWhenAbilityEnds)
		{
			StopPlayingMontage(OverrideBlendOutTimeForStopWhenEndAbility);
		}
	}

	Super::OnDestroy(AbilityEnded);
}

bool UASAT_PlayMontageForMeshAndWait::StopPlayingMontage(float OverrideBlendOutTime)
{
	if (Mesh == nullptr)
	{
		return false;
	}
	
	if (Ability == nullptr)
	{
		return false;
	}
	
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (ActorInfo == nullptr)
	{
		return false;
	}
	
	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (AnimInstance == nullptr)
	{
		return false;
	}

	// Check if the montage is still playing
	// The ability would have been interrupted, in which case we should automatically stop the montage
	UASAbilitySystemComponent* ASC = GetTargetASC();
	if (ASC && Ability)
	{
		if (ASC->GetAnimatingAbilityFromAnyMesh() == Ability
			&& ASC->GetCurrentMontageForMesh(Mesh) == MontageToPlay)
		{
			// Unbind delegates so they don't get called as well
			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay);
			if (MontageInstance)
			{
				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
			}

			ASC->CurrentMontageStopForMesh(Mesh, OverrideBlendOutTime);
			return true;
		}
	}
	
	return false;
}

UASAbilitySystemComponent* UASAT_PlayMontageForMeshAndWait::GetTargetASC()
{
	return Cast<UASAbilitySystemComponent>(AbilitySystemComponent);
}
