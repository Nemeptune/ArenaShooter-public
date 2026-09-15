// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ASAT_PlayMontageForMeshAndWait.generated.h"

class UASAbilitySystemComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FASPlayMontageForMeshAndWaitDelegate);

/**
 * PlayMontageForAndWait but for specific mesh
 */
UCLASS()
class ARENASHOOTER_API UASAT_PlayMontageForMeshAndWait : public UAbilityTask
{
	GENERATED_BODY()
public:

	UASAT_PlayMontageForMeshAndWait(const FObjectInitializer& ObjectInitializer);

	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual FString GetDebugString() const override;
	virtual void OnDestroy(bool AbilityEnded) override;

	UPROPERTY(BlueprintAssignable)
	FASPlayMontageForMeshAndWaitDelegate OnCompleted;
	UPROPERTY(BlueprintAssignable)
	FASPlayMontageForMeshAndWaitDelegate OnBlendOut;
	UPROPERTY(BlueprintAssignable)
	FASPlayMontageForMeshAndWaitDelegate OnInterrupted;
	UPROPERTY(BlueprintAssignable)
	FASPlayMontageForMeshAndWaitDelegate OnCancelled;

	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (DisplayName="PlayMontageForMeshAndWait", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UASAT_PlayMontageForMeshAndWait* PlayMontageForMeshAndWait(
	UGameplayAbility* OwningAbility,
	FName TaskInstanceName,
	USkeletalMeshComponent* InMesh,
	UAnimMontage* MontageToPlay,
	float Rate, FName StartSection,
	bool bStopWhenAbilityEnds,
	float AnimRootMotionTranslationScale,
	bool bReplicateMontage,
	float OverrideBlendOutTimeForCancelAbility,
	float OverrideBlendOutTimeForStopWhenEndAbility);
	
protected:
	
	// Mesh that the Montage is playing on. Must be owned by the AvatarActor.
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY()
	float Rate;

	UPROPERTY()
	FName StartSection;

	UPROPERTY()
	float AnimRootMotionTranslationScale;

	UPROPERTY()
	bool bStopWhenAbilityEnds;

	UPROPERTY()
	bool bReplicateMontage;

	UPROPERTY()
	float OverrideBlendOutTimeForCancelAbility;

	UPROPERTY()
	float OverrideBlendOutTimeForStopWhenEndAbility;

	/** Checks if the ability is playing a montage and stops that montage, returns true if a montage was stopped, false if not. */
	bool StopPlayingMontage(float OverrideBlendOutTime);

	/** Returns our ability system component */
	UASAbilitySystemComponent* GetTargetASC();

	UFUNCTION()
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	
	/** Callback function for when the owning Gameplay Ability is cancelled */
	UFUNCTION()
	void OnAbilityCancelled();
	
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle InterruptedHandle;
	
};
