// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "ASGameplayCueNotify_BuffAura.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

UCLASS(Abstract, Blueprintable)
class ARENASHOOTER_API AASGameplayCueNotify_BuffAura : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AASGameplayCueNotify_BuffAura();
	
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Aura")
	TObjectPtr<UNiagaraSystem> AuraFX3P;
	
	UPROPERTY(EditDefaultsOnly, Category = "Aura")
	FLinearColor AuraColor = FLinearColor::White;
	
	UPROPERTY(EditDefaultsOnly, Category = "Aura")
	TObjectPtr<UMaterialInterface> OverlayMaterial;
	
private:
	UNiagaraComponent* SpawnAura(UNiagaraSystem* System, USkeletalMeshComponent* Mesh) const;
	
	UMaterialInstanceDynamic* GetOverlayMID();
	
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> Spawned3P;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> OverlayMID;
};
