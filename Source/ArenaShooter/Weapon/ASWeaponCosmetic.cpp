// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ASWeaponCosmetic.h"
#include "Components/SkeletalMeshComponent.h"


AASWeaponCosmetic::AASWeaponCosmetic()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	SetRootComponent(WeaponRoot);

	WeaponMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh1P"));
	WeaponMesh1P->SetupAttachment(WeaponRoot);
	WeaponMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh1P->CastShadow = false;
	WeaponMesh1P->SetOnlyOwnerSee(true);
	WeaponMesh1P->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;

	WeaponMesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh3P"));
	WeaponMesh3P->SetupAttachment(WeaponRoot);
	WeaponMesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh3P->CastShadow = true;
	WeaponMesh3P->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
}

void AASWeaponCosmetic::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (WeaponMesh1P) { WeaponMesh1P->SetVisibility(false, true); }
	if (WeaponMesh3P) { WeaponMesh3P->SetVisibility(false, true); }
}

#if WITH_EDITOR
void AASWeaponCosmetic::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (WeaponMesh3P && WeaponMesh3P->GetAttachParent())
	{
		WeaponMesh3P->SetRelativeTransform(AttachOffset3P);
	}
}
#endif

FTransform AASWeaponCosmetic::GetMuzzleTransform(FName Socket) const
{
	const FName Wanted = Socket.IsNone() ? MuzzleSocketName : Socket;

	if (WeaponMesh1P && WeaponMesh1P->DoesSocketExist(Wanted))
	{
		return WeaponMesh1P->GetSocketTransform(Wanted);
	}

	return GetActorTransform();
}

void AASWeaponCosmetic::AttachTo(USkeletalMeshComponent* Mesh1P, FName Socket1P, USkeletalMeshComponent* Mesh3P, FName Socket3P)
{
	const FAttachmentTransformRules Rules = FAttachmentTransformRules::SnapToTargetIncludingScale;

	if (WeaponMesh1P && Mesh1P)
	{
		WeaponMesh1P->AttachToComponent(Mesh1P, Rules, Socket1P);
		WeaponMesh1P->SetRelativeTransform(AttachOffset1P);
		WeaponMesh1P->SetVisibility(true, true);
	}

	if (WeaponMesh3P && Mesh3P)
	{
		WeaponMesh3P->AttachToComponent(Mesh3P, Rules, Socket3P);
		WeaponMesh3P->SetRelativeTransform(AttachOffset3P);
		WeaponMesh3P->CastShadow = true;
		WeaponMesh3P->bCastHiddenShadow = true;
		WeaponMesh3P->SetVisibility(true, true);
	}
}

void AASWeaponCosmetic::DetachAndHide()
{
	const FDetachmentTransformRules Rules = FDetachmentTransformRules::KeepRelativeTransform;

	if (WeaponMesh1P)
	{
		WeaponMesh1P->DetachFromComponent(Rules);
		WeaponMesh1P->SetVisibility(false, true);
	}

	if (WeaponMesh3P)
	{
		WeaponMesh3P->DetachFromComponent(Rules);
		WeaponMesh3P->bCastHiddenShadow = false;
		WeaponMesh3P->SetVisibility(false, true);
	}
}
