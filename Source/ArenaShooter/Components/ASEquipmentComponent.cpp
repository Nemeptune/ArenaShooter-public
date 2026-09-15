// Fill out your copyright notice in the Description page of Project Settings.


#include "ASEquipmentComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ASGameplayTags.h"
#include "ASInventoryComponent.h"
#include "ASLogChannels.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/ASWeaponCosmetic.h"
#include "Weapon/ASWeaponDefinition.h"
#include "Weapon/ASWeaponHolder.h"
#include "Weapon/ASWeaponInstance.h"


UASEquipmentComponent::UASEquipmentComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UASEquipmentComponent::BindToInventory()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	UASInventoryComponent* Inv = UASInventoryComponent::FindInventoryComponent(Pawn ? Pawn->GetPlayerState() : nullptr);
	if (Inv == BoundInventory.Get())
	{
		return;
	}

	if (UASInventoryComponent* Old = BoundInventory.Get())
	{
		Old->OnActiveWeaponChanged.RemoveAll(this);
	}

	BoundInventory = Inv;

	if (!Inv)
	{
		EquipWeapon(nullptr);
		return;
	}
	
	Inv->OnActiveWeaponChanged.AddUObject(this, &UASEquipmentComponent::EquipWeapon);
	EquipWeapon(Inv->GetActiveInstance());
}

void UASEquipmentComponent::EquipWeapon(UASWeaponInstance* Instance)
{
	if (EquippedWeapon == Instance)
	{
		return;
	}

	if (EquippedWeapon)
	{
		RemoveEquipTags();
		ClearEquipLock();
		DestroyCosmetic();
		EquippedWeapon->NotifyUnequipped();
	}

	EquippedWeapon = Instance;

	if (EquippedWeapon)
	{
		SpawnCosmetic();
		ApplyEquipTags();
		EquippedWeapon->NotifyEquipped(Cosmetic);
		PlayEquipMontages();
		PlayEquipSound();
	}
}

void UASEquipmentComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UASInventoryComponent* Inv = BoundInventory.Get())
	{
		Inv->OnActiveWeaponChanged.RemoveAll(this);
	}
	BoundInventory = nullptr;

	// Weapon instances hang off the PlayerState and outlive the body. A pawn that is no longer the
	// avatar (a corpse replaced by a respawn) must take down only what it owns, or it strips tags
	// and the cosmetic from the weapon the new body just equipped.
	if (IsLiveAvatar())
	{
		EquipWeapon(nullptr);   // tags off, lock cleared, cosmetic destroyed
	}
	else
	{
		DestroyCosmetic();
		EquippedWeapon = nullptr;
	}
	
	Super::EndPlay(Reason);
}

void UASEquipmentComponent::SpawnCosmetic()
{
	IASWeaponHolder* Holder = GetHolder();
	AActor* OwnerActor = GetOwner();
	const UASWeaponDefinition* Definition = EquippedWeapon ? EquippedWeapon->GetDefinition() : nullptr;

	if (!Holder || !OwnerActor || !Definition)
	{
		return;
	}

	// Layers come off the definition and need no gun actor — link first, so a missing
	// CosmeticClass can't leave the holder stuck in the ref pose.
	Holder->LinkAnimLayers(Definition->FPLinkedLayer, Definition->TPLinkedLayer);

	UWorld* World = GetWorld();
	if (!World || !Definition->CosmeticClass)
	{
		return;   // only the cue notifies read these meshes, and they never run there
	}

	AASWeaponCosmetic* NewCosmetic = World->SpawnActorDeferred<AASWeaponCosmetic>(
		Definition->CosmeticClass, FTransform::Identity, OwnerActor, Cast<APawn>(OwnerActor),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!NewCosmetic)
	{
		UE_LOG(LogAS, Warning, TEXT("Failed to spawn cosmetic for %s"), *GetNameSafe(Definition));
		return;
	}

	NewCosmetic->FinishSpawning(FTransform::Identity);
	NewCosmetic->AttachToActor(OwnerActor, FAttachmentTransformRules::KeepRelativeTransform);
	NewCosmetic->AttachTo(
		Holder->GetHolderMesh1P(), Holder->GetWeapon1PAttachPoint(),
		Holder->GetHolderMesh3P(), Holder->GetWeapon3PAttachPoint());

	Cosmetic = NewCosmetic;
}

void UASEquipmentComponent::DestroyCosmetic()
{
	if (Cosmetic)
	{
		Cosmetic->Destroy();
		Cosmetic = nullptr;
	}

	// Back to the unarmed layers.
	if (IASWeaponHolder* Holder = GetHolder())
	{
		Holder->LinkAnimLayers(nullptr, nullptr);
	}
}

void UASEquipmentComponent::PlayEquipMontages()
{
	IASWeaponHolder* Holder = GetHolder();
	const UASWeaponDefinition* Definition = EquippedWeapon ? EquippedWeapon->GetDefinition() : nullptr;
	if (!Holder || !Definition)
	{
		return;
	}

	float LockTime = 0.0f;

	if (UAnimMontage* Montage1P = Definition->Equip1PMontage)
	{
		USkeletalMeshComponent* Mesh1P = Holder->GetHolderMesh1P();
		if (UAnimInstance* Anim1P = Mesh1P ? Mesh1P->GetAnimInstance() : nullptr)
		{
			LockTime = Anim1P->Montage_Play(Montage1P);
		}
	}

	if (UAnimMontage* Montage3P = Definition->Equip3PMontage)
	{
		USkeletalMeshComponent* Mesh3P = Holder->GetHolderMesh3P();
		if (UAnimInstance* Anim3P = Mesh3P ? Mesh3P->GetAnimInstance() : nullptr)
		{
			Anim3P->Montage_Play(Montage3P);
		}
	}

	if (!IsAuthorityOrLocal())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetASC();
	UWorld* World = GetWorld();
	if (!ASC || !World)
	{
		return;
	}

	const FGameplayTag ChangingTag = FASGameplayTags::Ability_Weapon_IsChanging;

	if (LockTime <= 0.0f)
	{
		ASC->SetLooseGameplayTagCount(ChangingTag, 0);
		return;
	}

	ASC->SetLooseGameplayTagCount(ChangingTag, 1);

	TWeakObjectPtr<UASEquipmentComponent> WeakThis(this);
	World->GetTimerManager().SetTimer(EquipLockTimer, [WeakThis, ChangingTag]()
	{
		if (const UASEquipmentComponent* Self = WeakThis.Get())
		{
			if (UAbilitySystemComponent* LockASC = Self->GetASC())
			{
				LockASC->SetLooseGameplayTagCount(ChangingTag, 0);
			}
		}
	}, LockTime, false);
}

void UASEquipmentComponent::PlayEquipSound()
{
	IASWeaponHolder* Holder = GetHolder();
	const UASWeaponDefinition* Definition = EquippedWeapon ? EquippedWeapon->GetDefinition() : nullptr;
	if (!Holder || !Definition || !Definition->EquipSound)
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(GetOwner());
	const bool bFirstPerson = Pawn && Pawn->IsLocallyControlled();
	USkeletalMeshComponent* Mesh = bFirstPerson ? Holder->GetHolderMesh1P() : Holder->GetHolderMesh3P();
	if (!Mesh)
	{
		return;
	}

	const FName AttachPoint = bFirstPerson ? Holder->GetWeapon1PAttachPoint() : Holder->GetWeapon3PAttachPoint();
	UGameplayStatics::SpawnSoundAttached(Definition->EquipSound, Mesh, AttachPoint,
		FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
}

void UASEquipmentComponent::ApplyEquipTags()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !EquippedWeapon)
	{
		return;
	}

	ASC->RemoveLooseGameplayTag(FASGameplayTags::Weapon_None);
	ASC->AddLooseGameplayTag(EquippedWeapon->GetWeaponTag());
}

void UASEquipmentComponent::RemoveEquipTags()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer With(FASGameplayTags::Ability_Weapon);
	FGameplayTagContainer Without(FASGameplayTags::Ability_Weapon_IsChanging);
	ASC->CancelAbilities(&With, &Without);

	if (EquippedWeapon)
	{
		ASC->RemoveLooseGameplayTag(EquippedWeapon->GetWeaponTag());
	}
}

void UASEquipmentComponent::ClearEquipLock()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EquipLockTimer);
	}
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->SetLooseGameplayTagCount(FASGameplayTags::Ability_Weapon_IsChanging, 0);
	}
}

IASWeaponHolder* UASEquipmentComponent::GetHolder() const
{
	return Cast<IASWeaponHolder>(GetOwner());
}

UAbilitySystemComponent* UASEquipmentComponent::GetASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

bool UASEquipmentComponent::IsAuthorityOrLocal() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && (Pawn->HasAuthority() || Pawn->IsLocallyControlled());
}

bool UASEquipmentComponent::IsLiveAvatar() const
{
	const UAbilitySystemComponent* ASC = GetASC();
	return ASC && ASC->GetAvatarActor() == GetOwner();
}
