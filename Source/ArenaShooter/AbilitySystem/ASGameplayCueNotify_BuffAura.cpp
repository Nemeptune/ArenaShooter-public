// Fill out your copyright notice in the Description page of Project Settings.


#include "ASGameplayCueNotify_BuffAura.h"

#include "NiagaraFunctionLibrary.h"
#include "Weapon/ASWeaponHolder.h"


AASGameplayCueNotify_BuffAura::AASGameplayCueNotify_BuffAura()
{
	bAutoDestroyOnRemove = true;
}

UNiagaraComponent* AASGameplayCueNotify_BuffAura::SpawnAura(UNiagaraSystem* System, USkeletalMeshComponent* Mesh) const
{
	if (!System || !Mesh)
	{
		return nullptr;
	}
	
	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		System, Mesh, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator
		, EAttachLocation::SnapToTarget, false);
	
	if (Comp)
	{
		Comp->SetVariableLinearColor(FName("User.Color"), AuraColor);
	}
		
	return Comp;
}

bool AASGameplayCueNotify_BuffAura::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	IASWeaponHolder* Holder = Cast<IASWeaponHolder>(MyTarget);
	const APawn* Pawn = Cast<APawn>(MyTarget);
	if (!Holder || !Pawn)
	{
		return false;
	}
	const bool bLocal = Pawn->IsLocallyControlled();
	
	if (OverlayMaterial)
	{
		if (bLocal)
		{
			Holder->GetHolderMesh1P()->SetOverlayMaterial(GetOverlayMID());
		}
		else
		{
			Holder->GetHolderMesh3P()->SetOverlayMaterial(GetOverlayMID());
		}
	}
	
	if (!Spawned3P && !bLocal)
	{
		Spawned3P = SpawnAura(AuraFX3P, Holder->GetHolderMesh3P());
	}
	
	return false;
}

bool AASGameplayCueNotify_BuffAura::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{

	if (Spawned3P)
	{
		Spawned3P->Deactivate();
		Spawned3P->SetAutoDestroy(true);
	}

	
	if (IASWeaponHolder* Holder = Cast<IASWeaponHolder>(MyTarget))
	{
		if (USkeletalMeshComponent* Mesh3P = Holder->GetHolderMesh3P())
		{
			Mesh3P->SetOverlayMaterial(nullptr);
		}
		if (USkeletalMeshComponent* Mesh1P = Holder->GetHolderMesh1P())
		{
			Mesh1P->SetOverlayMaterial(nullptr);
		}
	}

	Spawned3P = nullptr;
	return false;
}

UMaterialInstanceDynamic* AASGameplayCueNotify_BuffAura::GetOverlayMID()
{
	if (OverlayMaterial && !OverlayMID)
	{
		OverlayMID = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
		OverlayMID->SetVectorParameterValue(FName("Base Color"), AuraColor);
		OverlayMID->SetVectorParameterValue(FName("Secondary Color"), AuraColor);
	}
	
	return OverlayMID;
}
