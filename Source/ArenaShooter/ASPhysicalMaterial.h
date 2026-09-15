// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ASPhysicalMaterial.generated.h"


UCLASS()
class ARENASHOOTER_API UASPhysicalMaterial : public UPhysicalMaterial
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PhysicalProperties)
	FGameplayTagContainer Tags;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PhysicalProperties, meta = (ClampMin = 0.f))
	float DamageMultiplier = 1.f;
	
	static float GetDamageMultiplier(const UPhysicalMaterial* PhysMat)
	{
		const UASPhysicalMaterial* Zone = Cast<const UASPhysicalMaterial>(PhysMat);
		return Zone ? Zone->DamageMultiplier : 1.f;
	}
};
