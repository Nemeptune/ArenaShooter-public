// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASWeaponCosmetic.h"
#include "ASWeaponCosmetic_RocketLauncher.generated.h"

UCLASS()
class ARENASHOOTER_API AASWeaponCosmetic_RocketLauncher : public AASWeaponCosmetic
{
	GENERATED_BODY()

public:
	AASWeaponCosmetic_RocketLauncher();

	/** Shows or hides the loaded round on both meshes. */
	void SetLoadedRoundVisible(bool bVisible);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArenaShooter|RocketLauncher")
	TObjectPtr<USkeletalMeshComponent> LoadedRoundMesh1P;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArenaShooter|RocketLauncher")
	TObjectPtr<USkeletalMeshComponent> LoadedRoundMesh3P;
};
