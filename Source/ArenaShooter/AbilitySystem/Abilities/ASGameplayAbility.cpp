// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameplayAbility.h"

#include "Character/ASCharacter.h"
#include "System/ASGameplayTags.h"
#include "System/ASLogChannels.h"
#include "Player/ASPlayerController.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "Inventory/ASInventoryComponent.h"
#include "GameFramework/PlayerState.h"
#include "Weapon/ASWeaponInstance.h"

UASGameplayAbility::UASGameplayAbility()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	ActivationPolicy = EAbilityActivationPolicy::OnInputTriggered;
}

UASAbilitySystemComponent* UASGameplayAbility::GetASAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<UASAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

AASPlayerController* UASGameplayAbility::GetASPlayerControllerFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AASPlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr);
}

AController* UASGameplayAbility::GetControllerFromActorInfo() const
{
	if (CurrentActorInfo)
	{
		if (AController* PC = CurrentActorInfo->PlayerController.Get())
		{
			return PC;
		}
		// Look for a player controller or pawn in the owner chain.
		AActor* TestActor = CurrentActorInfo->OwnerActor.Get();
		while (TestActor)
		{
			if (AController* C = Cast<AController>(TestActor))
			{
				return C;
			}

			if (APawn* Pawn = Cast<APawn>(TestActor))
			{
				return Pawn->GetController();
			}

			TestActor = TestActor->GetOwner();
		}
	}
	return nullptr;
}

AASCharacter* UASGameplayAbility::GetASCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AASCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

UASInventoryComponent* UASGameplayAbility::GetInventoryFromActorInfo() const
{
	const APlayerState* PS = CurrentActorInfo ? Cast<APlayerState>(CurrentActorInfo->OwnerActor.Get()) : nullptr;
	return UASInventoryComponent::FindInventoryComponent(PS);
}

void UASGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (ActivationPolicy == EAbilityActivationPolicy::OnSpawn)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
	}
}

bool UASGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UASGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

UAnimMontage* UASGameplayAbility::GetCurrentMontageForMesh(USkeletalMeshComponent* InMesh)
{
	FAbilityMeshMontage AbilityMeshMontage;
	if (FindAbillityMeshMontage(InMesh, AbilityMeshMontage))
	{
		return AbilityMeshMontage.Montage;
	}
	return nullptr;
}

void UASGameplayAbility::SetCurrentMontageForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* InCurrentMontage)
{
	ensure(IsInstantiated());

	FAbilityMeshMontage AbilityMeshMontage;
	if (FindAbillityMeshMontage(InMesh, AbilityMeshMontage))
	{
		AbilityMeshMontage.Montage = InCurrentMontage;
	}
	else
	{
		CurrentAbilityMeshMontages.Add(FAbilityMeshMontage(InMesh,InCurrentMontage));
	}
}

bool UASGameplayAbility::FindAbillityMeshMontage(USkeletalMeshComponent* InMesh, FAbilityMeshMontage& InAbilityMontage)
{
	for (FAbilityMeshMontage& MeshMontage : CurrentAbilityMeshMontages)
	{
		if (MeshMontage.Mesh == InMesh)
		{
			InAbilityMontage = MeshMontage;
			return true;
		}
	}
	
	return false;
}
