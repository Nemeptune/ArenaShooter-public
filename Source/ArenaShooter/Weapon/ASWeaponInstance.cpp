// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ASWeaponInstance.h"

#include "System/ASGameplayTags.h"
#include "System/ASLogChannels.h"
#include "ASWeaponCosmetic.h"
#include "ASWeaponDefinition.h"
#include "AbilitySystem/ASAbilitySystemComponent.h"
#include "Inventory/ASInventoryComponent.h"
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#include "Net/UnrealNetwork.h"

UWorld* UASWeaponInstance::GetWorld() const
{
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

void UASWeaponInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UASWeaponInstance, Definition);
	DOREPLIFETIME_CONDITION(UASWeaponInstance, Ammo, COND_OwnerOnly);
}

void UASWeaponInstance::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}

UASAbilitySystemComponent* UASWeaponInstance::GetASC() const
{
	return Cast<UASAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningActor()));
}

AActor* UASWeaponInstance::GetOwningActor() const
{
	return Cast<AActor>(GetOuter());
}

UASInventoryComponent* UASWeaponInstance::GetOwningInventory() const
{
	const AActor* Owner = GetOwningActor();
	return Owner ? Owner->FindComponentByClass<UASInventoryComponent>() : nullptr;
}

bool UASWeaponInstance::HasAuthority() const
{
	const AActor* Owner = GetOwningActor();
	return Owner && Owner->HasAuthority();
}

bool UASWeaponInstance::IsLocallyControlled() const
{
	const AActor* Owner = GetOwningActor();
	return Owner && Owner->HasLocalNetOwner();
}

void UASWeaponInstance::Initialize(UASWeaponDefinition* InDefinition)
{
	Definition = InDefinition;
	Ammo = Definition ? Definition->StartingAmmo : 0;
	LastRepAmmo = Ammo;

	if (!Definition)
	{
		UE_LOG(LogAS, Warning, TEXT("UASWeaponInstance::Initialize with no definition"));
		return;
	}
	
	if (Definition->AbilitySet)
	{
		if (UASAbilitySystemComponent* ASC = GetASC())
		{
			Definition->AbilitySet->GiveToAbilitySystem(ASC, &GrantedHandles, this);
		}
		else
		{
			UE_LOG(LogAS, Error, TEXT("UASWeaponInstance::Initialize: no ASC on outer %s"), *GetNameSafe(GetOuter()));
		}
	}
}

void UASWeaponInstance::Uninitialize()
{
	if (UASAbilitySystemComponent* ASC = GetASC())
	{
		GrantedHandles.TakeFromAbilitySystem(ASC);
	}

	Definition = nullptr;
	Ammo = 0;
}

void UASWeaponInstance::NotifyEquipped(AASWeaponCosmetic* InCosmetic)
{
	LocalCosmetic = InCosmetic;
	OnEquipped();
}

void UASWeaponInstance::NotifyUnequipped()
{
	OnUnequipped();
	LocalCosmetic = nullptr;
}

void UASWeaponInstance::OnRep_Ammo()
{
	const int32 Confirmed = FMath::Max(0, LastRepAmmo - Ammo);
	PendingShots = FMath::Max(0, PendingShots - Confirmed);
	LastRepAmmo = Ammo;
	
	if (FPlatformTime::Seconds() - LastLocalFire > 0.5)
	{
		PendingShots = 0;
	}
	
	NotifyAmmoChanged();
}

void UASWeaponInstance::NotifyAmmoChanged()
{
	OnAmmoChanged.Broadcast(this);
}

int32 UASWeaponInstance::GetAmmo() const
{
	return FMath::Max(0, Ammo - PendingShots);
}

int32 UASWeaponInstance::GetFireCost() const
{
	return Definition  ? FMath::Max(1, Definition->FireCost) : 1;
}

float UASWeaponInstance::GetBaseDamage() const
{
	return GetDefinition() ? GetDefinition()->BaseDamage : 0.f;
}

void UASWeaponInstance::AddAmmo(int32 Delta)
{
	if (Delta <= 0 || !HasAuthority())
	{
		return;
	}

	Ammo += Delta;
	NotifyAmmoChanged();
}

void UASWeaponInstance::ConsumeAmmo(int32 Amount)
{
	if (Amount <= 0 || !HasAuthority())
	{
		return;
	}
	Ammo = FMath::Max(0, Ammo - Amount);
	NotifyAmmoChanged();
}

void UASWeaponInstance::PredictSpendAmmo(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	
	PendingShots += Amount;
	LastLocalFire = FPlatformTime::Seconds();
	NotifyAmmoChanged();
}

bool UASWeaponInstance::IsActive() const
{
	const UASInventoryComponent* Inventory = GetOwningInventory();
	return Inventory && Inventory->GetActiveInstance() == this;
}

bool UASWeaponInstance::CanFire() const
{
	const UWorld* World = GetWorld();
	if (!World || !Definition || Definition->FireInterval <= 0.f)
	{
		return true;
	}

	// The server measures the same interval the client does, but sees it through network jitter,
	// so give remote authority a fixed tolerance. Fixed rather than proportional: 
	// jitter is absolute, so a ratio would hand a slow weapon far too much slack.
	static constexpr float RemoteFireTolerance = 0.03f;
	const bool bIsRemoteAuthority = HasAuthority() && !IsLocallyControlled();
	const float Required = FMath::Max(0.f,Definition->FireInterval - (bIsRemoteAuthority ? RemoteFireTolerance : 0.f));

	return (World->GetTimeSeconds() - TimeLastFired) >= Required;
}

void UASWeaponInstance::MarkFired()
{
	if (const UWorld* World = GetWorld())
	{
		TimeLastFired = World->GetTimeSeconds();
	}
	
	OnFired();
}

const UASWeaponDefinition* UASWeaponInstance::GetDefinition() const
{
	return Definition;
}

FGameplayTag UASWeaponInstance::GetWeaponTag() const
{
	return Definition ? Definition->WeaponTag : FASGameplayTags::Weapon_None;
}

FGameplayTag UASWeaponInstance::GetAmmoType() const
{
	return Definition ? Definition->AmmoType : FGameplayTag();
}

const FSlateBrush& UASWeaponInstance::GetIcon() const
{
	static const FSlateBrush EmptyBrush;
	return Definition ? Definition->Icon : EmptyBrush;
}

int32 UASWeaponInstance::GetBulletsPerShot() const
{
	return Definition ? Definition->BulletsPerShot : 1;
}

float UASWeaponInstance::GetSpreadHalfAngleDeg() const
{
	return Definition ? Definition->SpreadHalfAngleDeg : 0.f;
}

USkeletalMeshComponent* UASWeaponInstance::GetWeaponMesh1P() const
{
	AASWeaponCosmetic* Cosmetic = LocalCosmetic.Get();
	return Cosmetic ? Cosmetic->GetWeaponMesh1P() : nullptr;
}

USkeletalMeshComponent* UASWeaponInstance::GetWeaponMesh3P() const
{
	AASWeaponCosmetic* Cosmetic = LocalCosmetic.Get();
	return Cosmetic ? Cosmetic->GetWeaponMesh3P() : nullptr;
}

FTransform UASWeaponInstance::GetMuzzleTransform(FName Socket) const
{
	AASWeaponCosmetic* Cosmetic = LocalCosmetic.Get();
	return Cosmetic ? Cosmetic->GetMuzzleTransform(Socket) : FTransform::Identity;
}