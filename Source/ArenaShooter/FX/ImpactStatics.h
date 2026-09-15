// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ImpactStatics.generated.h"

class UNiagaraSystem;
class UNiagaraDataChannelAsset;

UCLASS()
class ARENASHOOTER_API UImpactStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Writes blocking hits to a GameplayBurst channel once per handler system. Empty Systems = the channel's DefaultSystemToSpawn. */
	static void SpawnImpactFX(const UObject* WorldContextObject, const UNiagaraDataChannelAsset* Channel, TConstArrayView<TObjectPtr<UNiagaraSystem>> Systems,TConstArrayView<FHitResult> Hits, const FVector& MuzzlePosition);
};
