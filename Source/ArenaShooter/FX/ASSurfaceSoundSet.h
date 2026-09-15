// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ASSurfaceSoundSet.generated.h"

class USoundBase;

UCLASS(BlueprintType, Const)
class ARENASHOOTER_API UASSurfaceSoundSet : public UDataAsset
{
	GENERATED_BODY()
	
public:
	USoundBase* GetSound(EPhysicalSurface Surface) const
	{
		const TObjectPtr<USoundBase>* Found = SurfaceSounds.Find(Surface);
		return Found && *Found ? Found->Get() : DefaultSound.Get();
	}

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> DefaultSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<USoundBase>> SurfaceSounds;
};
