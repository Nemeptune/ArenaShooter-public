// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ASWeaponHolder.generated.h"

UINTERFACE()
class UASWeaponHolder : public UInterface
{
	GENERATED_BODY()
};


class ARENASHOOTER_API IASWeaponHolder
{
	GENERATED_BODY()

public:
	virtual USkeletalMeshComponent* GetHolderMesh1P() const = 0;
	virtual USkeletalMeshComponent* GetHolderMesh3P() const = 0;
	virtual FName GetWeapon1PAttachPoint() const = 0;
	virtual FName GetWeapon3PAttachPoint() const = 0;
	virtual void LinkAnimLayers(TSubclassOf<UAnimInstance> FPLayer, TSubclassOf<UAnimInstance> TPLayer) = 0;
};
