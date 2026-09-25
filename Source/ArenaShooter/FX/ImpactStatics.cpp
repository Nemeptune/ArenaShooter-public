// Fill out your copyright notice in the Description page of Project Settings.


#include "ImpactStatics.h"
#include "NiagaraDataChannel.h"
#include "NiagaraDataChannelAccessor.h"
#include "NiagaraDataChannelPublic.h"
#include "NiagaraDataChannel_GameplayBurst.h"
#include "NiagaraParameterBinding.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/HitResult.h"

namespace
{
	const FName NAME_ImpactPosition = FName("ImpactPosition");
	const FName NAME_ImpactNormal = FName("ImpactNormal");
	const FName NAME_ImpactSurface = FName("ImpactSurface");
	const FName NAME_MuzzlePosition = FName("MuzzlePosition");
	
	using FCellHits = TArray<const FHitResult*, TInlineAllocator<16>>;
	
	void WriteCell(UWorld* World, const UNiagaraDataChannelAsset* Channel, UNiagaraSystem* System, const FCellHits& Hits, const FVector& MuzzlePosition)
	{
		FNDCAccessContextInst AccessContext(Channel->Get()->GetAccessContextType());
		FNDCAccessContext_GameplayBurst& Context = AccessContext.GetChecked<FNDCAccessContext_GameplayBurst>();
		Context.Location = Hits[0]->ImpactPoint;
		Context.bOverrideLocation = true;
		
		if (System)
		{
			Context.SystemToSpawn = System;
			Context.bOverrideSystemToSpawn = true;
		}
		
		UNiagaraDataChannelWriter* Writer = UNiagaraDataChannelLibrary::WriteToNiagaraDataChannel_WithContext(World, Channel, AccessContext
			, Hits.Num(), false, true, true, TEXT("WeaponImpact"));
		if (!Writer)
		{
			return;
		}
		
		for (int32 Index = 0; Index < Hits.Num(); ++Index)
		{
			const FHitResult& Hit = *Hits[Index];
			const EPhysicalSurface Surface = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
			Writer->WritePosition(NAME_ImpactPosition, Index, Hit.ImpactPoint);
			Writer->WriteVector(NAME_ImpactNormal, Index, Hit.ImpactNormal);
			Writer->WriteEnum(NAME_ImpactSurface, Index, static_cast<uint8>(Surface));
			Writer->WritePosition(NAME_MuzzlePosition, Index, MuzzlePosition);
		}
	}
}

void UImpactStatics::SpawnImpactFX(const UObject* WorldContextObject, const UNiagaraDataChannelAsset* Channel, TConstArrayView<TObjectPtr<UNiagaraSystem>> Systems,TConstArrayView<FHitResult> Hits, const FVector& MuzzlePosition)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UImpactStatics::SpawnImpactFX);
	
	const UNiagaraDataChannel_GameplayBurst* BurstChannel = Channel ? Cast<UNiagaraDataChannel_GameplayBurst>(Channel->Get()) : nullptr;
	if (!BurstChannel || Hits.IsEmpty())
	{
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	
	const FVector InvCellSize = BurstChannel->GetGridCellSize().Reciprocal();
	TMap<FIntVector, FCellHits, TInlineSetAllocator<4>> HitsByCell;

	for (const FHitResult& Hit : Hits)
	{
		if (Hit.bBlockingHit)
		{
			const FVector Cell = Hit.ImpactPoint * InvCellSize;
			HitsByCell.FindOrAdd(FIntVector(FMath::FloorToInt32(Cell.X), FMath::FloorToInt32(Cell.Y), FMath::FloorToInt32(Cell.Z))).Add(&Hit);
		}
	}
	
	for (const TPair<FIntVector, FCellHits>& CellHits : HitsByCell)
	{
		if (Systems.IsEmpty())
		{
			WriteCell(World, Channel, nullptr, CellHits.Value, MuzzlePosition);
		}
		
		for (const TObjectPtr<UNiagaraSystem>& System : Systems)
		{
			if (System)
			{
				WriteCell(World, Channel, System, CellHits.Value, MuzzlePosition);
			}
		}
	}
}
