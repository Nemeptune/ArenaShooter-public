// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KnockbackStatics.generated.h"

struct FMomentumParams;
/**
 * 
 */
UCLASS()
class ARENASHOOTER_API UKnockbackStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static void ApplyKnockbackToActor(
		AActor* Target,
		const FVector& Origin,
		float Falloff,
		const FMomentumParams& Params,
		bool bIsSelfInflicted);
};
