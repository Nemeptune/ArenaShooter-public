// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ASPickup_Effect.h"
#include "ASPickup_Buff.generated.h"

class UASBuffDefinition;
class UNiagaraSystem;
class URotatingMovementComponent;
class UNiagaraComponent;

UCLASS()
class ARENASHOOTER_API AASPickup_Buff : public AASPickup_Effect
{
	GENERATED_BODY()

public:
	AASPickup_Buff();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void OnActiveStateChanged(bool bPlayTransitionFX) override;
	virtual bool CanGiveTo(UAbilitySystemComponent* ASC) override;

	void ApplyFXParameters(UNiagaraComponent* Component) const;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool GiveTo(UAbilitySystemComponent* ASC) override;

	const UASBuffDefinition* GetActiveBuff() const;
	void PickNewBuff();

	UPROPERTY(EditAnywhere, Category="ArenaShooter|Pickup")
	TArray<TObjectPtr<UASBuffDefinition>> PossibleBuffs;
	
	UPROPERTY(EditAnywhere, Category="ArenaShooter|Pickup")
	bool bAvoidRepeatingLastBuff = true;
	
	UPROPERTY(ReplicatedUsing = OnRep_ActiveBuffIndex)
	int32 ActiveBuffIndex = INDEX_NONE;
	
	UFUNCTION()
	void OnRep_ActiveBuffIndex();
	
	UPROPERTY(VisibleAnywhere, Category="ArenaShooter|Pickup|FX")
	TObjectPtr<UNiagaraComponent> IdleFX;

	UPROPERTY(VisibleAnywhere, Category="ArenaShooter|Pickup")
	TObjectPtr<URotatingMovementComponent> RotatingMovement;

	UPROPERTY(EditDefaultsOnly, Category="ArenaShooter|Pickup|FX")
	TObjectPtr<UNiagaraSystem> PickedUpFX;    // NS_Pickup_Success

	UPROPERTY(EditDefaultsOnly, Category="ArenaShooter|Pickup|FX")
	TObjectPtr<UNiagaraSystem> RespawnFX;     // NS_Pickup_Spawn
};
