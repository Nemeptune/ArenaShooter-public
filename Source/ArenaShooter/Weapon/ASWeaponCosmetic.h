// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASWeaponCosmetic.generated.h"

class USkeletalMeshComponent;

// The visible gun. Meshes and attach data
UCLASS()
class ARENASHOOTER_API AASWeaponCosmetic : public AActor
{
	GENERATED_BODY()
	
public:
	AASWeaponCosmetic();

	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	USkeletalMeshComponent* GetWeaponMesh1P() const { return WeaponMesh1P; }

	UFUNCTION(BlueprintPure, Category = "ArenaShooter|Weapon")
	USkeletalMeshComponent* GetWeaponMesh3P() const { return WeaponMesh3P; }

	FTransform GetMuzzleTransform(FName Socket) const;
	
	void AttachTo(USkeletalMeshComponent* Mesh1P, FName Socket1P, USkeletalMeshComponent* Mesh3P, FName Socket3P);
	void DetachAndHide();

protected:
	virtual void PostInitializeComponents() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh1P;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArenaShooter|Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh3P;

	UPROPERTY(EditAnywhere, Category = "ArenaShooter|Weapon|Attach")
	FTransform AttachOffset1P;

	UPROPERTY(EditAnywhere, Category = "ArenaShooter|Weapon|Attach")
	FTransform AttachOffset3P;

	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Weapon|Mesh")
	FName MuzzleSocketName = TEXT("Muzzle");
};
