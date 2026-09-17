// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ASGameMode.generated.h"

class UAbilitySystemComponent;
class UASWeaponDefinition;

UCLASS()
class ARENASHOOTER_API AASGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;
	
protected:
	void GiveLoadout(UAbilitySystemComponent* AbilitySystemComponent);
	
	void StripLoadout(UAbilitySystemComponent* AbilitySystemComponent);
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Loadout")
	TArray<TObjectPtr<UASWeaponDefinition>> DefaultLoadout;
	
	UPROPERTY(EditDefaultsOnly, Category = "ArenaShooter|Loadout", meta = (ClampMin = 0))
	int32 InitialSlot = 0;
};
