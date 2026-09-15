// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ArenaShooter.h"
#include "GameplayCueNotify_Actor.h"
#include "ASGameplayCueNotify_BeamActor.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

/**
 * Looping beam cue.
 */
UCLASS()
class ARENASHOOTER_API AASGameplayCueNotify_BeamActor : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AASGameplayCueNotify_BeamActor();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual void Tick(float DeltaTime) override; // for Beam end update only

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	TObjectPtr<UNiagaraSystem> BeamSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	FName MuzzleSocketName = "Muzzle";

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	FName BeamEndParamName = "BeamEnd";

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	float TraceRange = 10000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	TEnumAsByte<ECollisionChannel> TraceChannel = COLLISION_WEAPON;

	UPROPERTY(EditDefaultsOnly, Category = "Beam|Audio")
	TObjectPtr<USoundBase> FireLoopSound1P;

	UPROPERTY(EditDefaultsOnly, Category = "Beam|Audio")
	TObjectPtr<USoundBase> FireLoopSound3P; 
	
	UPROPERTY(EditDefaultsOnly, Category = "Beam|Audio")
	FName IntensityParamName = "Intensity";

	UPROPERTY(EditDefaultsOnly, Category = "Beam|Audio", meta = (ClampMin = 0, Units = "Seconds"))
	float IntensityRampTime = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Beam|Audio", meta = (ClampMin = 0, Units = "Seconds"))
	float StopFadeTime = 0.08f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Beam|Audio")
	TObjectPtr<USoundBase> FireStartSound;     
	
	UPROPERTY(EditDefaultsOnly, Category = "Beam|Audio")
	TObjectPtr<USoundBase> FireStopSound;   

private:
	FVector ComputeBeamEnd() const;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> BeamComp = nullptr;

	UPROPERTY()
	TObjectPtr<UAudioComponent> FireAudioComp = nullptr;

	TWeakObjectPtr<APawn> TargetPawn;
	
	TWeakObjectPtr<USkeletalMeshComponent> WeaponMeshWeak;
	float ActiveTime = 0.f;
};
