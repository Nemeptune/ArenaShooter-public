// Fill out your copyright notice in the Description page of Project Settings.


#include "ASWeaponCosmetic_RocketLauncher.h"


// Sets default values
AASWeaponCosmetic_RocketLauncher::AASWeaponCosmetic_RocketLauncher()
{
	LoadedRoundMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LoadedRoundMesh1P"));
	LoadedRoundMesh1P->SetupAttachment(WeaponMesh1P, MuzzleSocketName);
	LoadedRoundMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LoadedRoundMesh1P->CastShadow = false;
	LoadedRoundMesh1P->SetOnlyOwnerSee(true);
	LoadedRoundMesh1P->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;

	LoadedRoundMesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LoadedRoundMesh3P"));
	LoadedRoundMesh3P->SetupAttachment(WeaponMesh3P, MuzzleSocketName);
	LoadedRoundMesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LoadedRoundMesh3P->CastShadow = true;
	LoadedRoundMesh3P->SetOwnerNoSee(true);
}

void AASWeaponCosmetic_RocketLauncher::SetLoadedRoundVisible(bool bVisible)
{
	if (LoadedRoundMesh1P)
	{
		LoadedRoundMesh1P->SetVisibility(bVisible);
	}
	if (LoadedRoundMesh3P)
	{
		LoadedRoundMesh3P->SetVisibility(bVisible);
	}
}
