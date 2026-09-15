// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "FX/ImpactStatics.h"
#include "ASGameplayCueNotify_WeaponFireActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UNiagaraDataChannelAsset;

UCLASS(Abstract, Blueprintable)
class ARENASHOOTER_API AASGameplayCueNotify_WeaponFireActor : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AASGameplayCueNotify_WeaponFireActor();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool Recycle() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX")
	TObjectPtr<UNiagaraSystem> MuzzleFlashFX;

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX")
	TObjectPtr<UNiagaraSystem> TracerFX;

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX")
	TObjectPtr<UNiagaraSystem> ShellEjectFX;

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX")
	TObjectPtr<UStaticMesh> ShellEjectMesh;

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX|Audio")
    TObjectPtr<USoundBase> FireSound1P;
    
    UPROPERTY(EditDefaultsOnly, Category = "WeaponFX|Audio")
    TObjectPtr<USoundBase> FireSound3P;

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX")
	FName MuzzleSocketName = "Muzzle";

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX|Niagara Params")
	FName TriggerParamName = "User.Trigger";

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX|Niagara Params")
	FName ImpactPositionsName = "User.ImpactPositions";

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX|Niagara Params")
	FName MuzzleDirectionName = "User.Direction";

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX|Niagara Params")
	FName ShellMeshParamName = "User.ShellEjectStaticMesh";

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX")
	float IdleDestroyTime = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX")
	float IdleCheckInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX|Impacts")
	TObjectPtr<UNiagaraDataChannelAsset> ImpactChannel;
	
	UPROPERTY(EditDefaultsOnly, Category = "WeaponFX|Impacts")
	TArray<TObjectPtr<UNiagaraSystem>> ImpactSystems;
	
private:
	UNiagaraComponent* CreateFXComponent(UNiagaraSystem* System);
	void BindToMesh(USkeletalMeshComponent* MeshComp, bool bFirstPerson);
	void WakeFX();
	void TickIdle();
	bool AreAllSystemsInactive() const;
	void ReleaseFX();
	bool EnsureAwake(UNiagaraComponent* Comp, bool& State, bool bForceRestart = false);
	void Flip(UNiagaraComponent* Comp, bool& State);

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> MuzzleFlashComp;
	
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> TracerComp;
	
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ShellEjectComp;
	
	TWeakObjectPtr<USkeletalMeshComponent> BoundMesh;

	TArray<FVector> ImpactPositions;

	bool bMuzzleFlashTrigger = false;
	bool bTracerTrigger = false;
	bool bShellEjectTrigger = false;

	/** True once the systems have been told to stop spawning and we are waiting for particles to die. */
	bool bWindingDown = false;

	float LastFireTime = 0.f;

	FTimerHandle IdleTimerHandle;
	
	TArray<FHitResult> ImpactHits;
};
